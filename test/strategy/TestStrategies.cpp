//
// Created by Rory McStay on 06/07/2021.
//
#define _TURN_OFF_PLATFORM_STRING
#include "BreakOutStrategy.h"
#include "fwk/TestMarketData.h"
#define _TURN_OFF_PLATFORM_STRING
#include "fwk/TestOrdersApi.h"
#include "fwk/TestPositionApi.h"

#define REGISTER(strategy_)                                                    \
  extern "C" std::shared_ptr<BreakOutStrategy<TestOrdersApi, TestPositionApi>> \
  RegisterBreakOutStrategy(std::shared_ptr<TestMarketData> mdPtr_,             \
                           std::shared_ptr<TestOrdersApi> od_,                 \
                           std::shared_ptr<TestPositionApi> positionApi_,      \
                           std::shared_ptr<InstrumentService> insSvc_) {       \
    auto sharedPtr =                                                           \
        std::make_shared<BreakOutStrategy<TestOrdersApi, TestPositionApi>>(    \
            mdPtr_, od_, positionApi_, insSvc_);                               \
    return sharedPtr;                                                          \
  }

REGISTER(BreakOutStrategy)
