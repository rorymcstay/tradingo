find_package(benchmark REQUIRED)

add_executable(
        benchmarkTradingo
        data_parsing.cpp
        ../../src/Signal.cpp
        ../../src/signal/MovingAverageCrossOver.cpp
        ../../src/MarketData.cpp
        ../TestMarketData.cpp
)

target_link_libraries(benchmarkTradingo
        test_trading_funcs
        ${GTEST_LIBRARY}
        benchmark::benchmark_main)
