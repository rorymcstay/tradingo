cmake_minimum_required(VERSION 3.19)
project(trading_bot)

set(CMAKE_CXX_STANDARD 14)
include_directories(thirdparty/cpprestsdk/Release/include/)


#### Third party ####
#set(CPPREST_ROOT ${CMAKE_INSTALL_PREFIX})
find_package(CPPRESTSDK REQUIRED)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
find_package(Boost REQUIRED)
add_subdirectory(thirdparty/api-connectors/auto-generated/cpprest)
add_subdirectory(thirdparty/cpprestsdk)


include_directories(thirdparty/api-connectors/auto-generated/cpprest)
add_executable(trading_bot main.cpp)
target_link_libraries(trading_bot CppRestSwaggerClient)
