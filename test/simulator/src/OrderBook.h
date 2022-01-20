#ifndef SIMULATOR_ORDERBOOK_H
#define SIMULATOR_ORDERBOOK_H
#include <algorithm>
#include <ctime>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>


#include "Domain.h"
#include "Utils.h"


template<typename T>
class OrderBook
{

    using TOrdPtr = typename Order<T>::Ptr;
    using TExecPtr = typename ExecReport<T>::Ptr;

    using OrderQueue = typename std::priority_queue<
          typename Order<T>::Ptr,
          typename std::vector<typename Order<T>::Ptr>,
          OrderCompare<T>>;
    using Traders=std::unordered_map<int, Trader::Ptr>;
    using RootOrderMap=std::unordered_map<int, TOrdPtr>;

    OrderQueue                  _buyOrders;
    OrderQueue                  _sellOrders;
    RootOrderMap                _rootOrders;
    Traders                     _registeredTraders;
    price_t                     _tickSize;
    std::queue<TExecPtr>        _execReports;
    int                         _oidSeed;
    std::string                 _symbol;
    price_t                     _closePrice;
    std::vector<qty_t>          _buyLevels;
    std::vector<qty_t>          _sellLevels;
    std::mutex                  _mutex;
    bool                        _open;
    std::thread                 _matchingThread;
    qty_t                       _tradedVolume;

public:
    using Ptr = std::shared_ptr<OrderBook>;
    OrderBook(std::string  symbol_, price_t closePrice_);
    ~OrderBook();

    void onOrderSingle(TOrdPtr& order_);
    void onOrderCancelRequest(const TOrdPtr &order_);

private:
    void matchingRoutine();
    void updateLevel(Side side_, price_t price_, qty_t qty_);
    bool isTraderRegistered(int traderID_);
    Trader::Ptr registerTrader(int traderID_);
    void acceptNewOrderRequest(const TOrdPtr& order_);
    void rejectNewOrderRequest(const TOrdPtr& order_, const std::string& reason_);
    void rejectCancelRequest(const TOrdPtr& order_, const std::string& reason_);
    TOrdPtr findRootOrder(int orderID_);
    void addExecReport(const TExecPtr& report_);
    void onAmendDown(const TOrdPtr &order_, qty_t newQty_);

    void onTrade(TOrdPtr& buyOrder_, TOrdPtr& sellOrder_, price_t crossPx_, qty_t crossQty_);
    void onCancel(const TOrdPtr &order_);
    void match();
    static bool canCross(const TOrdPtr& buyOrder_, const TOrdPtr& sellOrder_);
    TOrdPtr getLiveOrder(Side side_);
    OrderQueue& getOrderQueue(Side side_);
    bool isTickAligned(price_t price_) const;
    bool isValidPrice(price_t price_) const;
public:
    price_t tickSize() const { return _tickSize; }
    const std::string& symbol() const { return _symbol; }
    qty_t qtyAtLevel(Side side_, price_t price_) const;
    price_t bestAsk() const;
    price_t bestBid() const;
    TExecPtr getExecMessage();
    qty_t tradedVolume() const { return _tradedVolume; }

    void start();
    void stop();


};

template<typename T>
OrderBook<T>::OrderBook(std::string  symbol_, price_t closePrice_)
:   _buyOrders()
,   _sellOrders()
,   _registeredTraders()
,   _tickSize(0.01)
,   _oidSeed(0)
,   _symbol(std::move(symbol_))
,   _closePrice(closePrice_)
,   _buyLevels(std::round(1/_tickSize)*20, 0)
,   _sellLevels(std::round(1/_tickSize)*20, 0)
,   _open(false)
,   _tradedVolume(0)
{}

template<typename T>
OrderBook<T>::~OrderBook() {
    _open = false;
    // wait for matching to stop
    _matchingThread.join();
}


template<typename T>
void OrderBook<T>::start()
{
    _open = true;
    _matchingThread = std::thread(&OrderBook<T>::matchingRoutine, this);
}


template<typename T>
void OrderBook<T>::stop()
{
    _open = false;
}


template<typename T>
void OrderBook<T>::matchingRoutine()
{
    INFO("Continuous trading start");
    while(_open)
    {
        match();
    }
    INFO("Continuous trading finish " << LOG_NVP("TotalVolume", _tradedVolume));
}


template<typename T>
bool OrderBook<T>::isTickAligned(price_t price_) const
{
    if (((int)std::round(price_*100) % (int)std::round(tickSize()*100)) != 0)
        return false;
    return true;
}

template<typename T>
bool OrderBook<T>::isValidPrice(price_t price_) const
{
    if (std::abs(_closePrice - price_) > 10)
        return false;
    return true;
}

template<typename T>
void OrderBook<T>::updateLevel(Side side_, price_t price_, qty_t qty_)
{
    auto& levels = side_ == Side::Buy ? _buyLevels : _sellLevels;
    // close and price must always be tick aligned, and price within 10 of closePrice
    price_t normaliser = std::max(_closePrice - 10, 0.0);
    size_t level = (price_ - normaliser)/_tickSize;
    levels[level] += qty_;
}

template<typename T>
qty_t OrderBook<T>::qtyAtLevel(Side side_, price_t price_) const
{
    auto& levels = side_ == Side::Buy ? _buyLevels : _sellLevels;
    if (price_ < 0 || price_ > levels.size())
        return 0;
    price_t normaliser = std::max(_closePrice - 10, 0.0);
    int index = (price_ - normaliser)/_tickSize;
    return levels[index];
}


template<typename T>
Trader::Ptr OrderBook<T>::registerTrader(int traderID_)
{
    auto trader = std::make_shared<Trader>();
    _registeredTraders.emplace(std::pair(traderID_, trader));
    return trader;
}

template<typename T>
bool OrderBook<T>::isTraderRegistered(int traderID_)
{
    if (_registeredTraders.find(traderID_) == _registeredTraders.end())
        return false;
    return true;
}

template<typename T>
typename OrderBook<T>::OrderQueue&
OrderBook<T>::getOrderQueue(Side side_)
{
    return (side_ == Side::Buy) ? _buyOrders : _sellOrders;
}

template<typename T>
void
OrderBook<T>::onOrderSingle(typename Order<T>::Ptr& order_)
{
    order_->setEntryTimeNow();
    order_->setorderID(++_oidSeed);
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto& orderQueue = getOrderQueue(order_->side());
    if (not isTickAligned(order_->price()))
    {
        INFO("Order price is not a multiple of ticksize" << LOG_VAR(order_->price()) << LOG_VAR(_tickSize));
        rejectNewOrderRequest(order_, "Order_price_is_not_multiple_of_ticksize");
        return;
    }
    if (not isValidPrice(order_->price()))
    {
        INFO("Order price is not a multiple of within threshold (10) of" << LOG_VAR(_closePrice) << LOG_VAR(order_->price()));
        rejectNewOrderRequest(order_, "Order_price_is_outside_threshold_of_closePrice");
        return;
    }
    auto traderID = order_->traderID();
    Trader::Ptr trader;
    if (not isTraderRegistered(traderID))
    {
        trader = registerTrader(traderID);
    }
    else
    {
        trader = _registeredTraders[order_->traderID()];
    }
    if (trader->isRateExceeded())
    {
        INFO("Message rate exceeded for " << LOG_NVP("traderID", order_->traderID()));
        rejectNewOrderRequest(order_, "Message_rate_exceeded");
        return;
    }
    acceptNewOrderRequest(order_);
    orderQueue.push(order_);
    updateLevel(order_->side(), order_->price(), order_->ordQty());
}

template<typename T>
void OrderBook<T>::rejectNewOrderRequest(const typename Order<T>::Ptr& order_, const std::string& reason)
{
    INFO("Rejecting new order request: " << LOG_NVP("OrderID",order_->orderID()) << LOG_VAR(reason)
        << LOG_NVP("TraderID", order_->traderID()));
    order_->setstatus(OrdStatus::Rejected);
    auto execReport = std::make_shared<ExecReport<T>>(order_, ExecType::Reject);
    execReport->settext(reason);
    addExecReport(execReport);
}

template<typename T>
void OrderBook<T>::acceptNewOrderRequest(const typename Order<T>::Ptr& order_)
{
    INFO("Accepting new order request: " << LOG_NVP("OrderID",order_->orderID())
        << LOG_NVP("Side", order_->side())  << LOG_NVP("Price", order_->price())
        << LOG_NVP("OrdQty", order_->ordQty()));
    order_->setstatus(OrdStatus::New);
    _rootOrders[order_->orderID()] = order_;
    addExecReport(std::make_shared<ExecReport<T>>(order_, ExecType::New));
}

template<typename T>
void OrderBook<T>::addExecReport(const typename ExecReport<T>::Ptr& execReport_)
{
    _execReports.push(execReport_);
}

template<typename T>
typename Order<T>::Ptr OrderBook<T>::findRootOrder(int orderID_)
{
    if (_rootOrders.find(orderID_) == _rootOrders.end())
        return nullptr;
    else
        return _rootOrders[orderID_];
}

template<typename T>
void OrderBook<T>::onCancel(const typename Order<T>::Ptr &order_)
{
    order_->setstatus(OrdStatus::Cancelled);
    addExecReport(std::make_shared<ExecReport<T>>(order_, ExecType::Cancel));
    _rootOrders.erase(order_->orderID());
    updateLevel(order_->side(), order_->price(), order_->leavesQty());
}

template<typename T>
void OrderBook<T>::onAmendDown(const typename Order<T>::Ptr &order_, qty_t newQty_)
{
    qty_t oldQty = order_->ordQty();
    order_->setordQty(newQty_);
    addExecReport(std::make_shared<ExecReport<T>>(order_, ExecType::Replaced));
    updateLevel(order_->side(), order_->price(), newQty_-oldQty);
}

template<typename T>
void OrderBook<T>::onOrderCancelRequest(const typename Order<T>::Ptr &order_)
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto traderID = order_->traderID();
    if (not isTraderRegistered(traderID))
    {
        rejectCancelRequest(order_, "Trader_not_registered.");
        return;
    }
    if (_registeredTraders[traderID]->isRateExceeded())
    {
        rejectCancelRequest(order_, "Message_rate_exceeded");
    }

    auto originalOrder = findRootOrder(order_->orderID());
    if (!originalOrder)
    {
        rejectCancelRequest(order_, "Order_not_found.");
        return;
    }
    qty_t newQty = order_->ordQty();
    qty_t oldQty = originalOrder->cumQty();
    if (newQty < oldQty)
    {
        rejectCancelRequest(originalOrder, "Quantity_amend_up_is_not_allowed");
        return;
    }

    if (newQty == 0)
    {
        onCancel(originalOrder);
    }
    else if (newQty < order_->cumQty())
    {
        rejectCancelRequest(originalOrder, "Too_late_to_cancel");
        onCancel(originalOrder);
    }
    else
    {
        onAmendDown(originalOrder, newQty);
    }
}


template<typename T>
bool OrderBook<T>::canCross(const typename Order<T>::Ptr& buy_, const typename Order<T>::Ptr& sell_)
{
    if (greater_equal(buy_->price(), sell_->price()))
        return true;
    else
        return false;
}


template<typename T>
typename ExecReport<T>::Ptr OrderBook<T>::getExecMessage()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    if (_execReports.empty())
        return nullptr;
    auto execReport =  _execReports.front();
    _execReports.pop();
    return execReport;
}


template<typename T>
void OrderBook<T>::onTrade(typename Order<T>::Ptr& buyOrder_, typename Order<T>::Ptr& sellOrder_, price_t crossPx_, qty_t crossQty_)
{
    INFO("Trade: " << LOG_NVP("Price", crossPx_) << LOG_NVP("Quantity", crossQty_));
    buyOrder_->setlastPrice(crossPx_);
    sellOrder_->setlastPrice(crossPx_);
    buyOrder_->setlastQty(crossQty_);
    sellOrder_->setlastQty(crossQty_);
    buyOrder_->setstatus(OrdStatus::PartiallyFilled);
    sellOrder_->setstatus(OrdStatus::PartiallyFilled);
    if (buyOrder_->leavesQty() == 0)
    {
        // finalise order
        buyOrder_->setstatus(OrdStatus::Filled);
        _buyOrders.pop();
    }
    if (sellOrder_->leavesQty() == 0)
    {
        // finalise order
        sellOrder_->setstatus(OrdStatus::Filled);
        _sellOrders.pop();
    }
    // send exec reports
    auto execReport = std::make_shared<ExecReport<T>>(sellOrder_, ExecType::Trade);
    addExecReport(execReport);
    execReport = std::make_shared<ExecReport<T>>(buyOrder_, ExecType::Trade);
    addExecReport(execReport);
    updateLevel(Side::Buy, buyOrder_->price(), -crossQty_);
    updateLevel(Side::Sell, sellOrder_->price(), -crossQty_);
    _tradedVolume+=crossQty_;
}


template<typename T>
typename Order<T>::Ptr
OrderBook<T>::getLiveOrder(Side side_)
{
    auto& orderQueue = getOrderQueue(side_);
    typename Order<T>::Ptr order;
    while(!orderQueue.empty())
    {
        order = orderQueue.top();
        if (not order)
            break;
        if (order->isCancelled())
        {
            orderQueue.pop();
            continue;
        }
        return order;
    }
    return nullptr;
}


template<typename T>
void OrderBook<T>::match()
{
    std::lock_guard<decltype(_mutex)> lock(_mutex);
    if (_buyOrders.empty() || _sellOrders.empty())
    {
        return;
    }
    auto buyOrder = getLiveOrder(Side::Buy);
    auto sellOrder = getLiveOrder(Side::Sell);
    if (!buyOrder || !sellOrder)
        return;
    if (canCross(buyOrder, sellOrder))
    {
        auto crossQty = std::min(buyOrder->leavesQty(), sellOrder->leavesQty());
        auto crossPx = std::min(buyOrder->price(), sellOrder->price());
        onTrade(buyOrder, sellOrder, crossPx, crossQty);
    }
}


template<typename T>
void OrderBook<T>::rejectCancelRequest(const typename Order<T>::Ptr& order_, const std::string& reason_)
{
    auto execReport = std::make_shared<ExecReport<T>>(order_, ExecType::CancelReject);
    execReport->settext(reason_);
    addExecReport(execReport);
}


template<typename T>
price_t OrderBook<T>::bestBid() const
{
    for (size_t i(_buyLevels.size()-1); i >= 0; i--)
    {
        if (_buyLevels[i] != 0)
            return (i*_tickSize)+std::max(_closePrice-10, 0.0);
    }
    return -1;
}


template<typename T>
price_t OrderBook<T>::bestAsk() const
{
    for (size_t i(0); i < _sellLevels.size(); i++)
    {
        if (_sellLevels[i] != 0)
            return (i*_tickSize)+std::max(_closePrice-10, 0.0);
    }
    return -1;
}
#endif
