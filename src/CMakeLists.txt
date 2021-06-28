message(STATUS "BITMEX_INCLUDE = ${BITMEX_INCLUDE}")
message(STATUS "ABSL_INCLUDE = ${ABSL_INCLUDE}")
message(STATUS "OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}")
message(STATUS "CPPREST_ROOT = ${CPPREST_ROOT}")

include_directories(${ABSL_INCLUDE} ${BITMEX_INCLUDE})
include_directories(${CPPREST_ROOT}/include)
include_directories(${Boost_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})

add_library(trading_funcs
        Event.cpp
        MarketData.cpp
        BatchWriter.cpp
        OrderInterface.cpp
        ObjectPool.h
        ObjectPool.hpp
        # templated
        Strategy.h
        Config.cpp)

target_link_libraries(trading_funcs
        CppRestSwaggerClient
        boost_system
        pthread
        ${OPENSSL_CRYPTO_LIBRARY}
        absl::strings)

install(TARGETS
            trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
