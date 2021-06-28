cmake_minimum_required(VERSION 3.16)
project(Tradingo)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(test)
