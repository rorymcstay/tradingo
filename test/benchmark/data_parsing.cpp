//
// Created by Rory McStay on 02/09/2021.
//
#define _TURN_OFF_PLATFORM_STRING

#include <benchmark/benchmark.h>
#include <memory>
#include "model/Quote.h"

#include "Utils.h"
#include "fwk/TestEnv.h"
#include "Config.h"
#include "Series.h"
#include "signal/MovingAverageCrossOver.h"

static void BM_read_quotes_json(benchmark::State& state) {
    // Perform setup here

    auto config = std::make_shared<Config>();

    for (auto kvp : std::initializer_list<std::pair<std::string, std::string>>({
        DEFAULT_ARGS
    }
         )
    ) config->set(kvp.first, kvp.second);

    std::ifstream quoteFile;
    auto marketdata = std::make_shared<TestMarketData>(config, nullptr);
    auto signal = std::make_shared<MovingAverageCrossOver>(1000, 8000);
    signal->init(config, marketdata);
    marketdata->setCallback([&](){ signal->update(); });
    quoteFile.open(config->get<std::string>("tickStorage")+"/quotes_XBTUSD.json");
    if (!quoteFile.is_open()) {
        throw std::runtime_error("No quotes file found.");
    }
    for (auto _ : state) {
        auto quote = getEvent<model::Quote>(quoteFile);
        *marketdata << quote;
    }
}
BENCHMARK(BM_read_quotes_json);
