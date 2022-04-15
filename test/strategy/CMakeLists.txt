message(STATUS "Configuring test strategy library")

FILE(GLOB Sources TestStrategies.cpp 
    ../../src/signal/*.cpp
    ../../src/Config.cpp
    ../../src/Config.h
    )
add_library(test_trading_strategies SHARED ${Sources})

include_directories(../)
include_directories(../../src/signal ../../src/strategy)

target_link_libraries(test_trading_strategies
    test_trading_funcs
    CppRestSwaggerClient
    trading_funcs
)
install(TARGETS
            test_trading_strategies
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
