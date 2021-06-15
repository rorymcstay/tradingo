include_directories(cpprestsdk/Release/include/)


#### Third party ####
#set(CPPREST_ROOT ${CMAKE_INSTALL_PREFIX})
find_package(CPPRESTSDK REQUIRED)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${WEBSOCKETPP_INCLUDE})

message("${WEBSOCKETPP_INCLUDE}")
find_package(Boost REQUIRED)

### BITMEX CPPRest swagger ###
add_subdirectory(api-connectors/auto-generated/cpprest)
include_directories(api-connectors/auto-generated/cpprest)
include_directories(abseil-cpp/absl/)

add_subdirectory(abseil-cpp)


add_subdirectory(websocketpp)
