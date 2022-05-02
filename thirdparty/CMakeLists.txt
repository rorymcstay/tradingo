#### Third party ####

# Installed packages
find_package(CPPRESTSDK REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(BITMEX_SWAGGER_CLIENT REQUIRED)
### AWS CPP SDK ###
if (WITH_AWS_CPP_SDK)
    find_package(AWSSDK REQUIRED COMPONENTS s3)
endif(WITH_AWS_CPP_SDK)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPRESTSDK_INCLUDE_DIR})
include_directories(${BOOST_INCLUDE_DIR})

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


