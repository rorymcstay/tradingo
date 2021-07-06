FILE(GLOB Sources *.cpp)
add_library(trading_strategies SHARED Strategies.cpp ${Sources})

target_link_libraries(trading_strategies trading_funcs)

include_directories(../)

install(TARGETS
        trading_strategies
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )