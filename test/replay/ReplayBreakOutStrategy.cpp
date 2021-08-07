//
// Created by Rory McStay on 13/07/2021.
//
#include <gtest/gtest.h>
#include "../fwk/TestEnv.h"

TEST(BreakOutStrategy, test_playback) {
    TestEnv env({
        {"symbol", "XBTUSD"},
        {"clOrdPrefix", "MCST"},
        {"factoryMethod", "RegisterBreakOutStrategy"},
        {"libraryLocation", "/usr/lib"},
        {"startingAmount", "1000"},
        {"referencePrice", "35000"},
        {"shortTermWindow", "1000"},
        {"longTermWindow", "100000"},
        {"logLevel", "info"},
        {"storage", "./"}
    });
    env.playback("trades_XBTUSD.json", "quotes_XBTUSD.json");
}
