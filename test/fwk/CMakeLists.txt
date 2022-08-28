FILE(GLOB SourcesToTest
        ../../src/*.cpp
        ../../src/strategy/*.cpp
        ../../signal/*.cpp)

link_directories(${CPPREST_ROOT}/lib)
message(STATUS "BITMEXAPI_INCLUDE_DIR = ${BITMEXAPI_INCLUDE_DIR}")
add_compile_definitions("TESTDATA_LOCATION=\"${CMAKE_INSTALL_PREFIX}/etc/test/data\"")

include_directories(
    ${BITMEXAPI_INCLUDE_DIR}/
    ${BITMEXAPI_INCLUDE_DIR}/api
    ${BITMEXAPI_INCLUDE_DIR}/model
)

add_library(test_trading_funcs
        ${SourcesToTest}
        # test framework
        TestOrdersApi.cpp
        TestPositionApi.cpp
        TestMarketData.cpp
        TestUtils.cpp
        TestEnv.cpp
        Params.cpp
        TempStorage.cpp
        TempStorage.h
)
set(TGT_LINK_LIBRARIES 
        pthread
        ${PACKAGE_INSTALLS}/swagger/lib/libCppRestSwaggerClient.a
        boost_system
        dl
        aws-cpp-sdk-s3
        ${OPENSSL_CRYPTO_LIBRARY})

target_link_libraries(test_trading_funcs
        #${CPPREST_LIBRARY}
        ${TGT_LINK_LIBRARIES}
)

install(TARGETS
            test_trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
