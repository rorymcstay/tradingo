#include <exception>
#include <gtest/gtest.h>

#include <map>
#include <sstream>
#include <memory>
#include <utility>

#include "OrderBook.h"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)
#define LN  " |line#:" TO_STRING(__LINE__)

using Params_data = std::map<std::string, std::string>;

class Params
{
    Params_data _data;
    std::string _paramString;
public:
    explicit Params(std::string  str_)
    :   _data()
    ,   _paramString(std::move(str_))
    {}
    std::string& operator[] (const std::string& key_)
    {
        return _data[key_];
    }
    std::string& at(const std::string& key_)
    {
        try
        {
            return _data.at(key_);
        }
        catch (std::exception& ex)
        {
            ERROR("Missing mandatory parameter! " << LOG_VAR(key_) << LOG_VAR(_paramString));
            throw ex;
        }
    }

    const std::string& at(const std::string& key_, const std::string& default_)
    {
        if (_data.find(key_) == _data.end())
        {
            return default_;
        }
        return at(key_);
    }
};

template<typename T>
struct EnvMessage
{
    typename Order<T>::Ptr order;
    typename ExecReport<T>::Ptr execReport;

    explicit EnvMessage(Params params_)
    {
        double price = std::stod(params_.at("Price"));
        int qty = std::stoi(params_.at("OrdQty"));
        Side side = str2enum<Side>(params_.at("Side").c_str());
        auto temporder = std::make_shared<Order<T>>(side, qty, price);
        if (params_["Type"] == "Newtypename Order<T>")
        {
            int traderID = std::stoi(params_.at("TraderID"));
            temporder->settraderID(traderID);
            order = temporder;
        }
        else if (params_["Type"] == "Canceltypename Order<T>")
        {
            int traderID = std::stoi(params_.at("TraderID"));
            int oid = std::stoi(params_.at("typename Order<T>ID"));
            temporder->settraderID(traderID);
            temporder->setorderID(oid);
            order = temporder;
        }
        else if (params_["Type"] == "typename ExecReport<T>")
        {
            int oid = std::stoi(params_.at("typename Order<T>ID"));
            temporder->setorderID(oid);
            auto execType = str2enum<ExecType>(params_.at("ExecType").c_str());
            execReport = std::make_shared<ExecReport<T>>(temporder, execType);
        }
        else
        {
            throw std::runtime_error("Unknown Type of test message " + params_["Type"]);
        }
    }
};

template<typename T>
class TestEnv
{

public:

    TestEnv(const std::string& symbol_, double closePrice_)
    : _orderBook(std::make_shared<OrderBook<T>>(symbol_, closePrice_))
    {
        _orderBook->start();
    }
    ~TestEnv()
    {
        _orderBook->stop();
    }

    const typename OrderBook<T>::Ptr& orderBook() const { return _orderBook; }

private:
    typename OrderBook<T>::Ptr _orderBook;

    static void messageFrom(const std::string& msgStr_, Params& params_)
    {
        Params params(msgStr_);
        std::string chunk;
        std::istringstream iss(msgStr_);
        while(std::getline(iss, chunk, ' '))
        {
            if (chunk == "NewOrder")
            {
                params["Type"] = chunk;
                continue;
            }
            else if (chunk == "ExecReport")
            {
                params["Type"] = chunk;
                continue;
            }
            else if (chunk == "CancelOrder")
            {
                params["Type"] = chunk;
                continue;
            }
            else if (chunk.find("line") != std::string::npos)
            {
                // ignore
                continue;
            }
            else if (chunk.find('=') == std::string::npos)
            {
                FAIL() << "Unknown chunk in param chunk=["+chunk+']';
            }
            size_t pos = chunk.find('=');
            std::string key = chunk.substr(0, pos);
            std::string val = chunk.substr(pos+1);
            params[key] = val;
        }
        params_ = params;
    }
public:
    // operator in <<
    EnvMessage<T> operator<< (const std::string& str_)
    {
        Params params(str_);
        messageFrom(str_, params);
        auto message = EnvMessage<T>(params);
        if (params["Type"] == "NewOrder")
            _orderBook->onOrderSingle(message.order);
        else if (params["Type"] == "CancelOrder")
            _orderBook->onOrderCancelRequest(message.order);
        return message;
    }

    // operator out >>
    void operator>> (const std::string& str_)
    {
        std::this_thread::sleep_for(std::chrono::microseconds(10));
        if (str_.find("NONE") != std::string::npos)
        {
            auto message = _orderBook->getExecMessage();
            if (message != nullptr)
            {
                FAIL() << str_ << " Unmatched event: " << LOG_NVP("OrderID", message->orderID())
                << LOG_NVP("Text", message->text()) << LOG_NVP("ExecType",message->execType())
                << LOG_NVP("Price",message->price()) << LOG_NVP("OrdQty", message->ordQty())
                << LOG_NVP("CumQty", message->cumQty()) << LOG_NVP("LastQty",message->lastQty())
                << LOG_NVP("LastQty", message->lastPrice()) << LOG_NVP("OrdStatus", message->ordStatus());
            }
            return;
        }

        Params params(str_);
        messageFrom(str_, params);

        typename ExecReport<T>::Ptr execReport = _orderBook->getExecMessage();
        if (!execReport)
        {
            FAIL() << "Unmatched filter: " <<  str_;
        }
        auto execType = params.at("ExecType");
        auto ordStatus = params.at("OrdStatus");
        ASSERT_EQ(execReport->price(), std::stod(params.at("Price"))) << str_;
        ASSERT_EQ(execReport->orderID(), std::stoi(params.at("OrderID"))) << str_;
        ASSERT_EQ(execReport->ordQty(), std::stoi(params.at("OrdQty"))) << str_;
        ASSERT_EQ(execReport->lastQty(), std::stoi(params.at("LastQty"))) << str_;
        ASSERT_EQ(execReport->cumQty(), std::stoi(params.at("CumQty"))) << str_;
        ASSERT_EQ(enum2str(execReport->execType()), execType) << str_;
        ASSERT_EQ(enum2str(execReport->ordStatus()), ordStatus) << str_;
        ASSERT_EQ(execReport->text(), params.at("Text", "")) << str_;
    }
};

