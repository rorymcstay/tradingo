#ifndef SIMULATOR_EXECWRITER_H
#define SIMULATOR_EXECWRITER_H
#include <ostream>
#include <utility>
#include <fstream>
#include <thread>

#include "Domain.h"
#include "OrderBook.h"


template<typename T>
class ExecWriter {

private:
    typename OrderBook<T>::Ptr _orderBookPtr;
    std::ofstream _fileHandle;
    std::string _fileLocation;
    size_t _batchSize;
    std::vector<typename ExecReport<T>::Ptr> _batch;
    std::thread _writerThread;
private:
    void main();
    void write(const typename ExecReport<T>::Ptr& message);
    void writeBatch();
    static std::string formatTime(timestamp_t time_);
public:
    ExecWriter(typename OrderBook<T>::Ptr  orderBook_);
    ~ExecWriter();
    void start();
};

template<typename T>
ExecWriter<T>::ExecWriter(typename OrderBook<T>::Ptr orderBook_)
:   _orderBookPtr(std::move(orderBook_))
,   _fileHandle()
,   _fileLocation("./"+_orderBookPtr->symbol()+"_exec_report.log")
,   _batchSize(5)
,   _batch()
{
}

template<typename T>
ExecWriter<T>::~ExecWriter()
{
    _orderBookPtr = nullptr;
    _writerThread.join();
    writeBatch();
    _fileHandle.close();
}


template<typename T>
std::string ExecWriter<T>::formatTime(timestamp_t time_)
{
    auto outTime = std::chrono::system_clock::to_time_t(time_);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&outTime), "%Y-%m-%d:%X");
    return ss.str();
}


template<typename T>
void ExecWriter<T>::write(const typename ExecReport<T>::Ptr &message)
{
    _fileHandle << LOG_NVP("ExecType", enum2str(message->execType()))
                << LOG_NVP("OrdStatus", enum2str(message->ordStatus()))
                << LOG_NVP("CumQty", message->cumQty())
                << LOG_NVP("OrdQty", message->ordQty())
                << LOG_NVP("LastPrice", message->lastPrice())
                << LOG_NVP("LastQty", message->lastQty())
                << LOG_NVP("OrderID", message->orderID())
                << LOG_NVP("Text", message->text())
                << LOG_NVP("TimeStamp", formatTime(message->timestamp())) << std::endl;
}


template<typename T>
void ExecWriter<T>::writeBatch()
{
    _fileHandle.open(_fileLocation, std::ios::app);
    for (auto& message : _batch)
        write(message);
    _fileHandle.close();
    _batch.clear();
}


template<typename T>
void ExecWriter<T>::main()
{
    while (_orderBookPtr)
    {
        typename ExecReport<T>::Ptr message = _orderBookPtr->getExecMessage();
        if (message) {
            _batch.emplace_back(message);
        }
        if (_batch.size() >= _batchSize)
            writeBatch();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

template<typename T>
void ExecWriter<T>::start()
{
    _writerThread = std::thread(&ExecWriter<T>::main, this);
}
#endif //SIMULATOR_EXECWRITER_H
