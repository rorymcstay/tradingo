#include "OrderInterface.h"

template<typename TOapi>
BitmexOrderManager<TOapi>::BitmexOrderManager(std::shared_ptr<MarketDataInterface> marketData_)
:   _marketData(std::move(marketData_))
{

}

template<typename TOapi>
void BitmexOrderManager<TOapi>::place_orders(const std::vector<model::Order>& orders_) {
    thread_local std::vector<std::string> orders;
    orders.clear();
    for (auto& order : orders_)
    {
        orders.push_back(order.toJson().serialize());
    }
    auto orderResp = _orderApi.order_newBulk(orders);
}
