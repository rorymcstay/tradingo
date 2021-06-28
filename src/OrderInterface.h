#ifndef TRADINGO_ORDERINTERFACE_H
#define TRADINGO_ORDERINTERFACE_H

#include <api/OrderApi.h>
#include <model/Order.h>
#include "MarketData.h"

using namespace io::swagger::client;

template<typename TOapi>
class OrderInterface
{
    TOapi _exchange;
public:

    OrderInterface();
    /*
    virtual void send_orders(const std::vector<model::Order>& orders_) = 0;
    virtual void cancel_order(const model::Order& order_) = 0;
    virtual void amend_order(const model::Order& order_) = 0;
    */
    TOapi& exchange() { return _exchange; }

};

template<typename TOapi>
OrderInterface<TOapi>::OrderInterface()
:   _exchange() {

}

template<typename TOapi>
class BitmexOrderManager
{
    std::shared_ptr<MarketDataInterface> _marketData;
    TOapi _orderApi;

public:
    explicit BitmexOrderManager(std::shared_ptr<MarketDataInterface> marketData_);
    void place_orders(const std::vector<model::Order>& orders_);
};


#endif // TRADINGO_ORDERINTERFACE_H