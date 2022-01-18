#ifndef TRADINGO_FUNCTIONAL_H
#define TRADINGO_FUNCTIONAL_H

#include <memory>
#include <string>

#include <api/InstrumentApi.h>
#include <model/Instrument.h>

#include "Allocation.h"
#include "Allocations.h"
#include "Utils.h"
using namespace io::swagger::client;

#define SEVENNULL boost::none,boost::none,boost::none,boost::none,boost::none,boost::none,boost::none

namespace func {


model::Instrument
get_instrument(const std::shared_ptr<api::InstrumentApi>& _instrumentApi, const std::string& symbol_);

price_t get_additional_cost(const std::shared_ptr<Allocation>& alloc_, double leverage_);

} // func


#endif //TRADINGO_FUNCTIONAL_H
