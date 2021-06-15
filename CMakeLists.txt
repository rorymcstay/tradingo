cmake_minimum_required(VERSION 3.19)
project(trading_bot)

add_subdirectory(thirdparty)
message(STATUS "BITMEX_INCLUDE = ${BITMEX_INCLUDE}")
message(STATUS "ABSL_INCLUDE = ${ABSL_INCLUDE}")
include_directories(${ABSL_INCLUDE} ${BITMEX_INCLUDE})

set(CMAKE_CXX_STANDARD 20)
add_library(trading_funcs Event.cpp MarketData.cpp BatchWriter.cpp InputParser.cpp InputParser.h)
target_link_libraries(trading_funcs CppRestSwaggerClient boost_system pthread crypto absl::strings)
add_executable(trading_bot main.cpp)

target_link_libraries(trading_bot trading_funcs)

install(TARGETS trading_bot CppRestSwaggerClient
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
