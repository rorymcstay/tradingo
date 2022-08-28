#include "OrderBookUtils.h"
#include "model/OrderBookL2.h"
#include <memory>

namespace tradingo_utils {
namespace orderbook_l2 {

OrderBook::OrderBook(
        const std::string& symbol_,
        price_t midPoint_,
        price_t tickSize_,
        price_t lotSize_
)
:   PriceLevelContainer<model::OrderBookL2>(
        midPoint_,
        tickSize_,
        lotSize_
    )
,   _symbol(symbol_)
{}

void OrderBook::removeLevel(price_t price_level_) {
    PriceLevelContainer::unoccupy(price_level_);
    get(price_level_)->setSize(0.0);
}

void OrderBook::updateLevel(
        price_t price_level_,
        const std::shared_ptr<model::OrderBookL2>& update_
    ) {
    auto level = PriceLevelContainer::occupy(price_level_);
    auto update_js = update_->toJson();
    level->fromJson(update_js);
}

} // orderbook_l2
} // tradingo_utils
