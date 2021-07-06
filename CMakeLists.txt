cmake_minimum_required(VERSION 3.16)
project(Tradingo)

set(CMAKE_CXX_STANDARD 20)
add_compile_options("-fPIC")

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(test)

message(STATUS "CMAKE_INSTALL_PREFIX = ${CMAKE_INSTALL_PREFIX}" )

set(CPACK_RPM_COMPONENT_INSTALL)

