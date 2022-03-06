enable_testing()

include(GoogleTest)

add_compile_definitions("LIBRARY_LOCATION=\"${CMAKE_INSTALL_PREFIX}/lib\"")

message(STATUS "GTEST_LIBRARY = ${GTEST_LIBRARY}")

link_directories(${CPPREST_ROOT}/lib)

include_directories("${gtest_SOURCE_DIR}/include")
include_directories(${CPPREST_ROOT}/include)
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
add_subdirectory(simulator)

add_executable(testTradingo

        # test files
        strategy/TestBreakOutStrategy.cpp

        TestSimpleMovingAverage.cpp
        TestBatchWriter.cpp
        TestAllocations.cpp
        TestStrategy.cpp
        TestMarketData.cpp
        TestConfig.cpp

        test_main.cpp
)

add_executable(testTradingo-lr

        TestSeries.cpp
        TestTimeControl.cpp
        test_api_OrderApi.cpp
        test_api_PositionApi.cpp

        test_main.cpp
)

add_test(NAME "tradingo-tests" COMMAND testTradingo)
add_test(NAME "tradingo-longrunning--tests" COMMAND testTradingo-lr)
add_test(NAME "orderbook-tests" COMMAND test_orderbook)


# install regression tests
file(GLOB files "regression/generated/*.json")
install(DIRECTORY
    regression/generated/ DESTINATION etc/test/regression/
    FILES_MATCHING
    PATTERN
    regression/generated/*.json
    PATTERN
    regression/*.json
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ
)
# add them to run
foreach(file ${files})
    add_test(NAME "regression-test-${file}"
        COMMAND tradingo_replay 
                    --config ${CMAKE_INSTALL_REFIX}/etc/test/regression/tradingo-cfg.json
                    --scenario-file ${CMAKE_INSTALL_REFIX}/etc/test/regression/${file}.json
    )
endforeach()

set(TEST_LINK_LIBRARIES 
        test_trading_funcs
        ${GTEST_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        CppRestSwaggerClient
        boost_system
        boost_filesystem
        pthread
)


target_link_libraries(testTradingo
    ${TEST_LINK_LIBRARIES}
)
target_link_libraries(testTradingo-lr
    ${TEST_LINK_LIBRARIES}
)

add_compile_definitions("TESTDATA_LOCATION=\"${CMAKE_INSTALL_PREFIX}/etc/test/data\"")
install(DIRECTORY
    data/ DESTINATION etc/test/data/
    FILES_MATCHING
    PATTERN
    data/*.json
    PATTERN
    data/objects/*.json
    PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ
    )

install(TARGETS
            testTradingo
            testTradingo-lr
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
