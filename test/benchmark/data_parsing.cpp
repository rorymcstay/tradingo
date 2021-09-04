//
// Created by Rory McStay on 02/09/2021.
//
#define _TURN_OFF_PLATFORM_STRING

#include <benchmark/benchmark.h>
#include "model/Quote.h"

#include "Utils.h"
#include "fwk/TestEnv.h"

static void BM_read_quotes_json(benchmark::State& state) {
    // Perform setup here
    std::ifstream quoteFile;
    quoteFile.open("data/quotes_XBTUSD.json");
    for (auto _ : state) {
        auto quote = getEvent<model::Quote>(quoteFile);
        LOGINFO( quote->toJson().serialize());
    }
}
BENCHMARK(BM_read_quotes_json);