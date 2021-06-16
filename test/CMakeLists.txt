add_executable(test_trading_funcs
        TestMarketData.cpp
        test_main.cpp
        )

include_directories(${PROJECT_SOURCE_DIR})

target_link_libraries(test_trading_funcs trading_funcs gtest pthread)
