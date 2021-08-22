find_package(GTest)
if(${GTest_FOUND} EQUAL "FALSE")
    message(FATAL "Gtest not found!")
endif(${GTest_FOUND} EQUAL "FALSE")

link_directories(${CPPREST_ROOT}/lib)

FILE(GLOB SourcesToTest ../src/*.cpp
        ../src/strategy/*.cpp
        ../signal/*.cpp)

add_library(test_trading_funcs
        ${SourcesToTest}

        # test framework
        fwk/TestOrdersApi.cpp
        fwk/TestMarketData.cpp
        fwk/TestUtils.cpp
        fwk/TestEnv.cpp
        fwk/Params.cpp
        TestConfig.cpp fwk/TempStorage.cpp fwk/TempStorage.h)

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

add_executable(replayTradingo
        replay/test_main.cpp)



include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${PROJECT_SOURCE_DIR}/test/)
include_directories(${BITMEX_INCLUDE})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPLOGGER_INCLUDES})

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#set (CMAKE_CXX_FLAGS "-std=c++11")

target_link_libraries(test_trading_funcs
        #${CPPREST_LIBRARY}
        pthread
        CppRestSwaggerClient
        boost_system
        dl
        ${OPENSSL_CRYPTO_LIBRARY}
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

if(${REPLAY_MODE})
    # REPLAY_MODE changes the following
    # TestEnv::replay(quote, trade) will
    # 1. manipulate the Signal class so that it wakes up on the market time
    # 2. play over the files as fast as possible.
    # with replay mode switched off, it will only dispatch events when the amount of time
    # is passed in the market is equal to the amount of actual time passed. This is to mimick
    # realtime trading as close as possible.
    target_compile_definitions(replayTradingo PUBLIC REPLAY_MODE)
endif(${REPLAY_MODE})

target_link_libraries(replayTradingo
        test_trading_funcs
        ${GTEST_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        CppRestSwaggerClient
        boost_system
        pthread
        boost_program_options
        )


add_subdirectory(strategy)

add_compile_definitions("LIBRARY_LOCATION=\"${CMAKE_INSTALL_PREFIX}/lib\"")

enable_testing()
add_test(NAME testTradingo COMMAND testTradingo)

install(TARGETS
            testTradingo
            replayTradingo
            test_trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
