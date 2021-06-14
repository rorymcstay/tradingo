include_directories(cpprestsdk/Release/include/)


#### Third party ####
#set(CPPREST_ROOT ${CMAKE_INSTALL_PREFIX})
find_package(CPPRESTSDK REQUIRED)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
find_package(Boost REQUIRED)

### BITMEX CPPRest swagger ###
add_subdirectory(api-connectors/auto-generated/cpprest)
include_directories(api-connectors/auto-generated/cpprest)


add_subdirectory(websocketpp)
