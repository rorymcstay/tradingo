#ifndef TRADINGO_FUNCTIONAL_H
#define TRADINGO_FUNCTIONAL_H

#include <memory>
#include <string>

#include <api/InstrumentApi.h>
#include <model/Instrument.h>
#include <model/Position.h>

#include "Allocation.h"
#include "Allocations.h"
#include "Utils.h"


#define SEVENNULL boost::none,boost::none,boost::none,boost::none,boost::none,boost::none,boost::none

namespace func {
using namespace io::swagger::client;

static const double SATOSHI = 10e7;




model::Instrument
get_instrument(const std::shared_ptr<api::InstrumentApi>& _instrumentApi, const std::string& symbol_);

price_t get_additional_cost(const std::shared_ptr<Allocation>& alloc_, double leverage_);

/// get the additional cost of changing position by qty_delta
price_t get_additional_cost(
        const std::shared_ptr<model::Position>& position_,
        qty_t qty_delta,
        price_t price);

price_t get_cost(price_t price_, qty_t qty_, double leverage_);

} // func


#endif //TRADINGO_FUNCTIONAL_H
