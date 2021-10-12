//
// Created by Rory McStay on 05/07/2021.
//

#include "BreakOutStrategy.h"
#include "api/OrderApi.h"

#define REGISTER(strategy_) \
extern "C" std::shared_ptr<strategy_<api::OrderApi>> Register##strategy_(std::shared_ptr<MarketDataInterface> mdPtr_, \
std::shared_ptr<api::OrderApi> od_, std::shared_ptr<InstrumentService> instSvc_) { \
    auto sharedPtr = std::make_shared<strategy_<api::OrderApi>>(mdPtr_, od_, instSvc_); \
    return sharedPtr; \
}

REGISTER(BreakOutStrategy)
