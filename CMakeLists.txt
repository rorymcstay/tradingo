cmake_minimum_required(VERSION 3.19)
project(trading_bot)

add_subdirectory(thirdparty)
set(CMAKE_CXX_STANDARD 14)

add_executable(trading_bot main.cpp)

target_link_libraries(trading_bot CppRestSwaggerClient)
