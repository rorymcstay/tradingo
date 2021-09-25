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

    auto config = std::make_shared<Config>();

    for (auto kvp : std::initializer_list<std::pair<std::string, std::string>>({
         {"symbol", "XBTUSD"},
         {"storage", "/tmp/"},
         {"tickStorage", "./data"},
         {"moving_average_crossover-callback", "false"},
         {"override-signal-callback", "true"},
         {"moving_average_crossover-interval", "1000"},
         {"shortTermWindow", "1000"},
         {"logLevel", "info"},
         {"referencePrice", "40000"},
         {"factoryMethod", "RegisterBreakOutStrategy"},
         {"longTermWindow", "8000"}}
         )
    ) config->set(kvp.first, kvp.second);
    auto env = TestEnv(config);

    std::ifstream quoteFile;
    quoteFile.open(config->get("tickStorage")+"/quotes_XBTUSD.json");
    if (!quoteFile.is_open()) {
        throw std::runtime_error("No quotes file found.");
    }
    auto marketdata = env.context()->marketData();
    auto signal = env.context()->strategy()->getSignal("moving_average_crossover");
    for (auto _ : state) {
        auto quote = getEvent<model::Quote>(quoteFile);
        *marketdata << quote;
    }
}
BENCHMARK(BM_read_quotes_json);
