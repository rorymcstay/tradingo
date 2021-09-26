#### Third party ####

# Installed packagesbb
find_package(CPPRESTSDK REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)

#set(CPPREST_LIBRARY ${CPPREST_ROOT}/lib/libcpprest.dylib PARENT_SCOPE)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPRESTSDK_INCLUDE_DIR})
include_directories(${BOOST_INCLUDE_DIR})

### BITMEX CPPRest swagger ###
add_subdirectory(api-connectors/auto-generated/cpprest)
set(BITMEX_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/api-connectors/auto-generated/cpprest PARENT_SCOPE)

message(STATUS "CPPRESTSDK_INCLUDE_DIR=${CPPRESTSDK_INCLUDE_DIR}")
message(STATUS "CPPRESTSDK_DIR=${CPPRESTSDK_DIR}")
### ABSL Libs ###
if(WITH_ABSL)
    add_subdirectory(abseil-cpp)
    set(ABSL_INCLUDE ${CMAKE_CURRENT_SOURCE_DIR}/abseil-cpp/absl/ PARENT_SCOPE)
endif(WITH_ABSL)

### CPPLogger ###
message(STATUS "Setting up aixlog headerfile")
set(CPPLOGGER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/aixlog/include PARENT_SCOPE)
set(CPPLOGGER_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/aixlog/include/aixlog.hpp PARENT_SCOPE)
include_directories(${CPPLOGGER_INCLUES})
message(STATUS "CPPLOGGER_INLCLUDES=${CPPLOGGER_INCLUDES}")

install(TARGETS
            CppRestSwaggerClient
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
