#include "Domain.h"
#include "Utils.h"
#include <ostream>


template<typename T>
std::ostream& operator << (std::ostream& is_, const typename Order<T>::Ptr& order_)
{
    is_ << LOG_NVP("Price", order_->price())
        << LOG_NVP("OrdQty", order_->ordQty())
        << LOG_NVP("Side", enum2str(order_->side()))
        << LOG_NVP("LeavesQty", order_->leavesQty())
        << LOG_NVP("CumQty", order_->cumQty())
        << LOG_NVP("LastQty", order_->lastQty())
        << LOG_NVP("LastPrice", order_->lastPrice())
        << LOG_NVP("OrdStatus", enum2str(order_->side()))
        << LOG_NVP("OrderID", order_->orderID())
        << LOG_NVP("TraderID", order_->traderID())
        ;
    return is_;
}

