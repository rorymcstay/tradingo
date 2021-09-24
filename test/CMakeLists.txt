enable_testing()

include(GoogleTest)

add_compile_definitions("LIBRARY_LOCATION=\"${CMAKE_INSTALL_PREFIX}/lib\"")

message(STATUS "GTEST_LIBRARY = ${GTEST_LIBRARY}")

link_directories(${CPPREST_ROOT}/lib)

include_directories("${gtest_SOURCE_DIR}/include")
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

add_test(NAME testTradingo COMMAND testTradingo)

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
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
