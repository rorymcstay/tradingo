#ifndef SIMULATOR_DOMAIN_H
#define SIMULATOR_DOMAIN_H
#include <chrono>
#include <ctime>
#include <string>
#include <memory>

#include "Utils.h"


ENUM_MACRO_7(OrdStatus,
    New,
    PendingNew,
    Cancelled,
    PendingCancel,
    PartiallyFilled,
    Filled,
    Rejected
)

ENUM_MACRO_2(Side,
    Buy,
    Sell
)

using timestamp_t = std::chrono::time_point<std::chrono::system_clock>;

using qty_t=double;
using price_t=double;

template<typename T>
class Order {
    Side        _side;
    OrdStatus   _status;
    qty_t       _ordQty;
    price_t      _price;
    std::string _symbol;
    timestamp_t _entryTime;
    std::string _orderID;
    qty_t       _cumQty;
    price_t     _lastPrice;
    qty_t       _lastQty;
    double      _traderID;

    std::shared_ptr<T> _orderData;

public:
    using Ptr=std::shared_ptr<Order>;
    Order(Side side_, qty_t ordQty_, price_t price_)
    :   _side(side_)
    ,   _status(OrdStatus::PendingNew)
    ,   _ordQty(ordQty_)
    ,   _price(price_)
    ,   _symbol("TEST")
    ,   _entryTime()
    ,   _orderID()
    ,   _cumQty(0)
    ,   _lastPrice(0)
    ,   _lastQty(0)
    ,   _traderID(0)
    {}

    void setOrderData(const std::shared_ptr<T> orderData_) {
        _orderData = orderData_;
    }

    OrdStatus status() const { 
        return str2enum<OrdStatus>(_orderData->getOrdStatus().c_str()); 
    }

    void setstatus(OrdStatus newStatus_) {
        _orderData->setOrdStatus(enum2str(newStatus_));
    }

    price_t price() const {
        return _orderData->getPrice();
    }

    Side side() const {
        return str2enum<Side>(_orderData->getSide().c_str());
    }

    const std::string& orderID() const {
        return _orderData->getOrderID(); 
    }

    void setorderID(const std::string& oid_) {
        _orderData->setOrderID(oid_);
    }

    double traderID() const {
        return _orderData->getAccount();
    }

    void settraderID(double traderID_) {
        _orderData->setAccount(traderID_);
    }

    void setEntryTimeNow() {
        _entryTime = std::chrono::system_clock::now();
    }

    timestamp_t entryTime() const { return _entryTime; }

    qty_t ordQty() const {
        return _orderData->getOrderQty();
    }

    void setordQty(qty_t newQty_) {
        _orderData->setOrderQty(newQty_);
    }

    qty_t cumQty() const {
        return _orderData->getCumQty();
    }
    qty_t leavesQty() const {
        return _orderData->getLeavesQty();
    }

    price_t avgPx() const {
        return _orderData->getAvgPx();
    }

    price_t lastPrice() const { return _orderData->getLastPx(); }

    void setlastPrice(price_t price_) {
        auto new_avg = (lastQty()/cumQty()) * price_ + ((cumQty() - lastQty())/cumQty()) * avgPx();
        _orderData->setAvgPx(new_avg);
    }

    qty_t lastQty() const {
        return _lastQty;
    }

    void setlastQty(qty_t qty_)
    {
        _orderData->setCumQty(_orderData->getCumQty() + qty_); 
        _orderData->setLeavesQty(_orderData->getLeavesQty() - qty_);
        _lastQty = qty_;
    }

    bool isCancelled() const { return status() == OrdStatus::Cancelled; }

};


ENUM_MACRO_6(ExecType, New, Trade, Cancel, Reject, CancelReject, Replaced)

template<typename T>
class ExecReport
{
    ExecType  _execType;
    price_t    _price;
    qty_t       _ordQty;
    OrdStatus _ordStatus;
    std::string _orderID;
    int _execID; 

    qty_t       _lastQty;
    qty_t       _cumQty;
    price_t    _lastPrice;
    std::string _text;
    timestamp_t _timestamp;
public:
    using Ptr = std::shared_ptr<ExecReport>;
    ExecReport(const typename Order<T>::Ptr& order_, ExecType execType_)
    :   _execType(execType_)
    ,   _price(order_->price())
    ,   _ordQty(order_->ordQty())
    ,   _ordStatus(order_->status())
    ,   _orderID(order_->orderID())
    ,   _execID(0)
    ,   _lastQty(order_->lastQty())
    ,   _cumQty(order_->cumQty())
    ,   _lastPrice(order_->lastPrice())
    ,   _text("")
    ,   _timestamp(std::chrono::system_clock::now())
    {
    }

    std::string orderID() const { return _orderID; }
    qty_t ordQty() const { return _ordQty; }
    price_t price() const { return _price; }
    int execID() const { return _execID; }
    ExecType execType() const { return _execType; }
    OrdStatus ordStatus() const { return _ordStatus; }
    price_t lastPrice() const { return _lastPrice; }
    qty_t lastQty() const { return _lastQty; }
    qty_t cumQty() const { return _cumQty; }
    void settext(const std::string& text_) { _text = text_; }
    const std::string& text() const { return _text; }
    timestamp_t timestamp() const { return _timestamp; }

};


class Trader
{
    int _messageCount;
    timestamp_t _lastMessageTime;

    qty_t _balance;
    qty_t _position;
 

public:
    using Ptr = std::shared_ptr<Trader>;
    Trader()
    :   _messageCount(0)
    ,   _lastMessageTime(std::chrono::system_clock::now()) {
    }

    void resetMessageCount() {
        _lastMessageTime = std::chrono::system_clock::now();
        _messageCount = 0;
    }

    bool isRateExceeded() {
        auto elapsed = std::chrono::system_clock::now() - _lastMessageTime;
        if (elapsed < std::chrono::seconds(1))
        {
            if (++_messageCount > 100)
                return true;
        }
        else
        {
            resetMessageCount();
        }
        return false;
    }
};


template<typename T>
class OrderCompare
{
public:

    bool operator () (
            const typename Order<T>::Ptr &self_,
            const typename Order<T>::Ptr &other_)
    {
        if (almost_equal(self_->price(), other_->price()))
        {
            // favour orders entered before other orders
            return self_->entryTime() > other_->entryTime();
        }
        return (self_->side() == Side::Buy) 
            ? less_than(self_->price(), other_->price()) 
            : greater_than(self_->price(), other_->price());
    }
};
#endif
