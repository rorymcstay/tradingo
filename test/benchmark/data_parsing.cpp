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
    //

    std::string tick_storage = std::getenv("TICK_STORAGE");
    std::string config_file = std::getenv("CONFIG_FILE");

    auto defaults = std::make_shared<Config>();

    for (auto kvp : std::initializer_list<std::pair<std::string, std::string>>({
        DEFAULT_ARGS,
        {"moving_average_crossover-callback", "true"},
        {"moving_average_crossover-interval", "1000"},
        {"moving_average_crossover-batch-size", "100000"},
        {"logLevel", "info"}
    }))
        defaults->set(kvp.first, kvp.second);

    auto config = std::make_shared<Config>(config_file);
    *defaults += *config;

    defaults->set("storage", "/tmp/benchmarking/");
    std::cout << defaults->toJson().serialize() << '\n';

    std::ifstream quoteFile;
    auto marketdata = std::make_shared<TestMarketData>(defaults, nullptr);
    auto signal = std::make_shared<MovingAverageCrossOver>(marketdata,
            defaults->get<double>("shortTermWindow"),
            defaults->get<double>("longTermWindow")
    );
    signal->init(defaults);
    marketdata->setCallback([&](){ signal->update(); });
    auto quote_file = defaults->get<std::string>("tickStorage")+"/quotes_XBTUSD.json";
    auto series = Series<model::Quote>(
            quote_file,
            /*resolution_=*/10000);
    for (auto _ : state) {
        for (auto quote : series) {
            *marketdata << quote;
        }
    }
}
BENCHMARK(BM_read_quotes_json);
