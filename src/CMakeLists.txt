message(STATUS "BITMEX_INCLUDE = ${BITMEX_INCLUDE}")
message(STATUS "ABSL_INCLUDE = ${ABSL_INCLUDE}")
include_directories(${ABSL_INCLUDE} ${BITMEX_INCLUDE})


add_library(trading_funcs
        Event.cpp
        MarketData.cpp
        BatchWriter.cpp
        InputParser.cpp
        InputParser.h
        ObjectPool.h
        ObjectPool.hpp)

target_link_libraries(trading_funcs CppRestSwaggerClient boost_system pthread crypto absl::strings)

install(TARGETS
            trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
