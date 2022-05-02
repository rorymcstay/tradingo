cmake_minimum_required(VERSION 3.16)
project(Tradingo)
set(VERSION "1.0.1")
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

# Get the latest abbreviated commit hash of the working branch
include(gitVersion)

set(CMAKE_CXX_STANDARD 20)
add_compile_options("-fPIC")
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

option(REPLAY_MODE "Build tradingo test pack" OFF)
option(WITH_AWS_CPP_SDK "Include aws s3 functions" ON)

enable_testing()

find_library(CPPREST_LIBRARY cpprest)

add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS=1)
message(STATUS "Disabled boost asio concepts.")

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(test)

message(STATUS "CPPRESTSDK_INCLUDE_DIR = ${CPPREST_ROOT}/include")
message(STATUS "CMAKE_INSTALL_PREFIX   = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "BITMEXAPI_INCLUDE_DIR  = ${BITMEXAPI_INCLUDE_DIR}")

project (my_project)

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "tradingo")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "rory@rorymcstay.com")
set(CPACK_PACKAGE_VENDOR "rorymcstay.com")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
