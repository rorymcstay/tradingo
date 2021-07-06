//
// Created by Rory McStay on 05/07/2021.
//

#include "BreakOutStrategy.h"
#include "api/OrderApi.h"


extern "C" std::shared_ptr<BreakOutStrategy<api::OrderApi>> RegisterBreakOutStrategy(std::shared_ptr<MarketDataInterface> mdPtr_,  std::shared_ptr<api::OrderApi> od_) {
    auto sharedPtr = std::make_shared<BreakOutStrategy<api::OrderApi>>(mdPtr_, od_);
    return sharedPtr;
}
