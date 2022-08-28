#### Third party ####
list(APPEND CMAKE_PREFIX_PATH "${PACKAGE_INSTALLS}/aws")
list(APPEND CMAKE_PREFIX_PATH "${PACKAGE_INSTALLS}/benchmark")
list(APPEND CMAKE_PREFIX_PATH "${PACKAGE_INSTALLS}/cpprest")
list(APPEND CMAKE_PREFIX_PATH "${PACKAGE_INSTALLS}/swagger")

# Installed packages
find_package(CPPRESTSDK REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Boost REQUIRED)
find_package(Threads REQUIRED)
find_package(AWSSDK REQUIRED COMPONENTS s3)


include(bitmex-swagger-client REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPRESTSDK_INCLUDE_DIR})
include_directories(${BOOST_INCLUDE_DIR})
set(BITMEX_INCLUDE "${PACKAGE_INSTALLS}/swagger/include" PARENT_SCOPE)
set(AWS_INCLUDE "${PACKAGE_INSTALLS}/aws/include" PARENT_SCOPE)
link_directories("${PACKAGE_INSTALLS}/swagger/lib")
#add_library(CppRestSwaggerClient STATIC IMPORTED)
#set_property(TARGET CppRestSwaggerClient PROPERTY
#    IMPORTED_LOCATION ${PACKAGE_INSTALLS}/swagger/lib/libCppRestSwaggerClient.a
#)

### CPPLogger ###
message(STATUS "Setting up aixlog headerfile")
set(CPPLOGGER_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/aixlog/include PARENT_SCOPE)
set(CPPLOGGER_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/aixlog/include/aixlog.hpp PARENT_SCOPE)
include_directories(${CPPLOGGER_INCLUES})
message(STATUS "CPPLOGGER_INLCLUDES=${CPPLOGGER_INCLUDES}")


