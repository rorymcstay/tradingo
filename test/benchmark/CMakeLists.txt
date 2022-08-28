find_library(benchmark REQUIRED)

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
        cpprest
        ${GTEST_LIBRARY}
        benchmark_main)

install(TARGETS
            benchmarkTradingo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
