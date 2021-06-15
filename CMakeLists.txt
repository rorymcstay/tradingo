cmake_minimum_required(VERSION 3.19)
project(trading_bot)



add_subdirectory(thirdparty)
set(CMAKE_CXX_STANDARD 20)

add_library(trading_funcs Event.cpp MarketData.cpp)

add_executable(trading_bot main.cpp)


target_link_libraries(trading_bot CppRestSwaggerClient boost_system pthread crypto trading_funcs absl::strings)

include_directories(thirdparty/api-connectors/auto-generated/cpprest)

install(TARGETS trading_bot CppRestSwaggerClient
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
)
