#### Third party ####

# Installed packages
find_package(CPPRESTSDK REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

### BITMEX CPPRest swagger ###
add_subdirectory(api-connectors/auto-generated/cpprest)
set(BITMEX_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/api-connectors/auto-generated/cpprest PARENT_SCOPE)

### ABSL Libs ###
add_subdirectory(abseil-cpp)
set(ABSL_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/abseil-cpp/absl/ PARENT_SCOPE)
