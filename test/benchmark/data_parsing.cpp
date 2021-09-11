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
#include "signal/MovingAverageCrossOver.h"

static void BM_read_quotes_json(benchmark::State& state) {
    // Perform setup here
    std::ifstream quoteFile;
    quoteFile.open("data/quotes_XBTUSD.json");
    auto config = std::make_shared<Config>();

    for (auto kvp : std::initializer_list<std::pair<std::string, std::string>>({
         {"symbol", "XBTUSD"},
         {"storage", "/tmp/"},
         {"moving_average_crossover-callback", "false"},
         {"override-signal-callback", "true"},
         {"moving_average_crossover-interval", "1000"},
         {"shortTermWindow", "1000"},
         {"longTermWindow", "8000"}}
         )
    ) config->set(kvp.first, kvp.second);

    auto marketdata = std::make_shared<TestMarketData>(config);
    auto signal = std::make_shared<MovingAverageCrossOver>(1000, 8000);
    signal->init(config, marketdata);
    marketdata->setCallback([&](){ signal->update(); });
    for (auto _ : state) {
        auto quote = getEvent<model::Quote>(quoteFile);
        *marketdata << quote;
    }
}
BENCHMARK(BM_read_quotes_json);
