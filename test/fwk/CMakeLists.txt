FILE(GLOB SourcesToTest
        ../../src/*.cpp
        ../../src/strategy/*.cpp
        ../../signal/*.cpp)

link_directories(${CPPREST_ROOT}/lib)


add_library(test_trading_funcs
        ${SourcesToTest}
        # test framework
        TestOrdersApi.cpp
        TestMarketData.cpp
        TestUtils.cpp
        TestEnv.cpp
        Params.cpp
        TempStorage.cpp
        TempStorage.h
        MarginCalculator.cpp)

target_link_libraries(test_trading_funcs
        #${CPPREST_LIBRARY}
        pthread
        CppRestSwaggerClient
        boost_system
        dl
        ${OPENSSL_CRYPTO_LIBRARY})
install(TARGETS
            test_trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
