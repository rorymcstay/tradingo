add_compile_definitions("LIBRARY_LOCATION=\"${CMAKE_INSTALL_PREFIX}/lib\"")
if(${REPLAY_MODE})
    message(STATUS "Replay mode is active")
    # REPLAY_MODE changes the following
    # TestEnv::replay(quote, trade) will
    # 1. manipulate the Signal class so that it wakes up on the market time
    # 2. play over the files as fast as possible.
    # with replay mode switched off, it will only dispatch events when the amount of time
    # is passed in the market is equal to the amount of actual time passed. This is to mimick
    # realtime trading as close as possible.
    target_compile_definitions(replayTradingo PUBLIC REPLAY_MODE)
endif(${REPLAY_MODE})

find_package(GTest REQUIRED)
link_directories(${CPPREST_ROOT}/lib)
include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/test/)
include_directories(${BITMEX_INCLUDE})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPLOGGER_INCLUDES})

add_subdirectory(fwk)
add_subdirectory(strategy)
add_subdirectory(benchmark)
add_subdirectory(replay)

add_executable(testTradingo

        # test files
        strategy/TestBreakOutStrategy.cpp

        test_api_OrderApi.cpp
        TestSimpleMovingAverage.cpp
        TestBatchWriter.cpp
        TestAllocations.cpp
        TestStrategy.cpp
        TestMarketData.cpp
        test_json.cpp
        test_main.cpp
        )

target_link_libraries(testTradingo
        test_trading_funcs
        ${GTEST_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        CppRestSwaggerClient
        boost_system
        boost_filesystem
        pthread
        )


install(TARGETS
            testTradingo
            benchmarkTradingo
            replayTradingo
            test_trading_funcs
            test_trading_strategies
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
