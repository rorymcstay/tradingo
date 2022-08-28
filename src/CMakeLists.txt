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
include_directories(${BITMEX_INCLUDE})
include_directories(${AWS_INCLUDE})

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(trading_funcs SHARED
        Event.cpp
        MarketData.cpp
        BatchWriter.cpp
        OrderInterface.cpp
        ObjectPool.h
        ObjectPool.hpp
        OrderBookUtils.cpp
        Strategy.h # templated
        Context.h # templated
        Series.h # templated
        Config.h # partial templating
        SimpleMovingAverage.cpp
        ExponentialMovingAverage.cpp
        Utils.cpp
        Allocation.cpp
        Allocations.cpp
        HeartBeat.cpp
		Signal.cpp
		CallbackTimer.cpp
		Functional.cpp
		InstrumentService.cpp

		signal/MovingAverageCrossOver.cpp

        #${CPPLOGGER_HEADER}
        )
link_directories(${CPPREST_ROOT}/lib)
link_directories(${PACKAGE_INSTALLS}/aws/lib)
link_directories(${PACKAGE_INSTALLS}/cpprest/lib)

list(APPEND 
    EXTRA_LINK_LIBRARIES
    aws-cpp-sdk-s3
)

# this should always be true and overrides REPLAY_MODE on non test libraries
target_compile_definitions(trading_funcs PUBLIC __REPLAY_MODE_GUARD)
target_link_libraries(trading_funcs
        cpprest
        boost_system
        pthread
        dl
        ${PACKAGE_INSTALLS}/swagger/lib/libCppRestSwaggerClient.a
        ${OPENSSL_CRYPTO_LIBRARY}
        ${EXTRA_LINK_LIBRARIES}
)


add_subdirectory(strategy)

install(TARGETS
            trading_funcs
        RUNTIME DESTINATION bin
		LIBRARY DESTINATION lib COMPONENT funcs)
