//
// Created by rory on 11/10/2021.
//

#ifndef TRADINGO_FUNCTIONAL_H
#define TRADINGO_FUNCTIONAL_H

#include "api/InstrumentApi.h"
#include "model/Instrument.h"

#include "Utils.h"
using namespace io::swagger::client;

#define SEVENNULL boost::none,boost::none,boost::none,boost::none,boost::none,boost::none,boost::none

namespace func {


std::shared_ptr<model::Instrument>
get_instrument(const std::shared_ptr<api::InstrumentApi>& _instrumentApi, const std::string& symbol_);

} // func


#endif //TRADINGO_FUNCTIONAL_H
