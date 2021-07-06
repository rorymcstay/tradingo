message(STATUS "Configuring test strategy library")
add_library(test_trading_strategies TestStrategies.cpp)
target_link_libraries(test_trading_strategies test_trading_funcs)
include_directories(../../src/strategy/)
include_directories(../)

install(TARGETS
        test_trading_strategies
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
