message(STATUS "BITMEX_INCLUDE = ${BITMEX_INCLUDE}")
message(STATUS "ABSL_INCLUDE = ${ABSL_INCLUDE}")
message(STATUS "OPENSSL_INCLUDE_DIR = ${OPENSSL_INCLUDE_DIR}")
message(STATUS "CPPREST_ROOT = ${CPPREST_ROOT}")
message(STATUS "CPPLOGGER_INCLUDES = ${CPPLOGGER_INCLUDES}")
message(STATUS "CPPLOGGER_HEADER = ${CPPLOGGER_HEADER}")

include_directories(${ABSL_INCLUDE} ${BITMEX_INCLUDE})
include_directories(${CPPREST_ROOT}/include)
include_directories(${Boost_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPLOGGER_INCLUDES})

add_library(trading_funcs SHARED
        Event.cpp
        MarketData.cpp
        BatchWriter.cpp
        OrderInterface.cpp
        ObjectPool.h
        ObjectPool.hpp
        Strategy.h # templated
        Context.h # templated
        Config.cpp
        SimpleMovingAverage.cpp

        # Signal.cpp
        #${CPPLOGGER_HEADER}
        )

target_link_libraries(trading_funcs
        CppRestSwaggerClient
        boost_system
        pthread
        dl
        ${OPENSSL_CRYPTO_LIBRARY}
        absl::strings)

add_subdirectory(strategy)

install(TARGETS
            trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
