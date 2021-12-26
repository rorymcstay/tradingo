//
// Created by Rory McStay on 05/07/2021.
//

#include <api/OrderApi.h>
#include <api/PositionApi.h>
#include <memory>

#include "BreakOutStrategy.h"

#define REGISTER(strategy_)                                                    \
  extern "C" std::shared_ptr<strategy_<api::OrderApi, api::PositionApi>>       \
      Register##strategy_(std::shared_ptr<MarketDataInterface> mdPtr_,         \
                          std::shared_ptr<api::OrderApi> od_,                  \
                          std::shared_ptr<api::PositionApi> positionApi_,      \
                          std::shared_ptr<InstrumentService> instSvc_) {       \
    auto sharedPtr =                                                           \
        std::make_shared<strategy_<api::OrderApi, api::PositionApi>>(          \
            mdPtr_, od_, positionApi_, instSvc_);                              \
    return sharedPtr;                                                          \
  }

REGISTER(BreakOutStrategy)
