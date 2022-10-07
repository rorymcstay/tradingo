#ifndef ORDERBOOK_UTILS_H
#define ORDERBOOK_UTILS_H
#include <model/Instrument.h>
#include <model/OrderBookL2.h>
#include "Allocation.h"

#include "PriceLevelContainer.h"


namespace tradingo_utils {
namespace orderbook_l2 {

static const long MX = 100000000;

using index_t = long;

inline index_t level_id(
        index_t symbolIdx_,
        price_t tickSize_,
        price_t price_
) {
    return (MX * symbolIdx_) - (price_ / tickSize_);
}

inline price_t price(
        index_t symbolIdx_,
        index_t id_,
        price_t tickSize_
) {
    return (static_cast<double>((MX * symbolIdx_) - id_)) * tickSize_;
}

inline index_t symbolIdx(
        price_t price_,
        index_t id_,
        price_t tickSize_
) {
    return (1/static_cast<double>(MX))*((price_/tickSize_)+id_);
}


class OrderBook : public PriceLevelContainer<model::OrderBookL2> {

    std::string _symbol;

public:

    OrderBook(
        const std::string& symbol_,
        price_t midPoint_,
        price_t tickSize_,
        price_t lotSize_
    );
    void updateLevel(
            price_t price_level,
            const std::shared_ptr<model::OrderBookL2>& update_
    );
    void removeLevel(price_t price_level);

};


} // orderbook_l2
} // tradingo_utils
#endif // ORDERBOOK_UTILS_H
