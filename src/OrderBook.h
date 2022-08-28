#include <model/Instrument.h>
#include <model/OrderBookL2.h>
#include "Allocation.h"


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
    return ((MX * symbolIdx_) - id_) * tickSize_;
}

inline index_t symbolIdx(
        price_t price_,
        index_t id_,
        price_t tickSize_
) {
    return (1/static_cast<double>(MX))*((price_/tickSize_)+id_);
}
} // orderbook_l2
} // tradingo_utils
