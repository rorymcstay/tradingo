find_package(GTest)
if(${GTest_FOUND} EQUAL "FALSE")
    message(FATAL "Gtest not found!")
endif(${GTest_FOUND} EQUAL "FALSE")

link_directories(${CPPREST_ROOT}/lib)

FILE(GLOB SourcesToTest ../src/*.cpp)

add_library(test_trading_funcs
        ${SourcesToTest}

        # test framework
        fwk/TestOrdersApi.cpp
        fwk/TestMarketData.cpp
        fwk/TestUtils.cpp
        fwk/TestEnv.cpp
        fwk/Params.cpp
        )

add_executable(testTradingo

        # test files
        TestBitMexAPI.cpp
        test_api_OrderApi.cpp
        test_main.cpp
        )

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${BITMEX_INCLUDE})
include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${OPENSSL_INCLUDE_DIR})

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#set (CMAKE_CXX_FLAGS "-std=c++11")

target_link_libraries(test_trading_funcs
        #${CPPREST_LIBRARY}
        pthread
        #trading_funcs
        dl
        ${OPENSSL_CRYPTO_LIBRARY}
        )
target_link_libraries(testTradingo
        test_trading_funcs
        ${GTEST_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        CppRestSwaggerClient
        boost_system
        pthread
        )

add_subdirectory(strategy)


install(TARGETS
            testTradingo
            test_trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
