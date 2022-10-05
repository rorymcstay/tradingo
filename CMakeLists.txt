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
option(PACKAGE_INSTALLS "Path to compiled tpps" "")

enable_testing()
#find_library(CPPREST_LIBRARY cpprest)
find_package(CPPRESTSDK REQUIRED)

list(APPEND CMAKE_PREFIX_PATH 
    "${PACKAGE_INSTALLS}/aws/lib/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-crt-cpp/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-http/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-io/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/s2n/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-common/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-cal/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-compression/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-mqtt/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-auth/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-sdkutils/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-event-stream/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-checksums/cmake"
    "${PACKAGE_INSTALLS}/aws/lib/aws-c-s3/cmake"
    "${PACKAGE_INSTALLS}/swagger/lib"
    )
list(APPEND CMAKE_PREFIX_PATH 
)

find_package(AWSSDK REQUIRED COMPONENTS s3)

add_definitions(-DBOOST_ASIO_DISABLE_CONCEPTS=1)
message(STATUS "Disabled boost asio concepts.")

add_subdirectory(thirdparty)
add_subdirectory(src)
add_subdirectory(app)
add_subdirectory(test)

message(STATUS "CPPRESTSDK_INCLUDE_DIR = ${CPPREST_ROOT}/include")
message(STATUS "CMAKE_INSTALL_PREFIX   = ${CMAKE_INSTALL_PREFIX}")
message(STATUS "BITMEXAPI_INCLUDE_DIR  = ${BITMEXAPI_INCLUDE_DIR}")

set(CPACK_PACKAGE_VERSION ${VERSION})
set(CPACK_GENERATOR "RPM")
set(CPACK_PACKAGE_NAME "tradingo")
set(CPACK_PACKAGE_RELEASE 1)
set(CPACK_PACKAGE_CONTACT "rory@rorymcstay.com")
set(CPACK_PACKAGE_VENDOR "rorymcstay.com")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
include(CPack)
