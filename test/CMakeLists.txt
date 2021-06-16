find_package(GTest)
if(${GTest_FOUND} EQUAL "FALSE")
    message(FATAL "Gtest not found!")
endif(${GTest_FOUND} EQUAL "FALSE")

add_executable(test_trading_funcs
        TestMarketData.cpp
        test_main.cpp
        )

include_directories(${PROJECT_SOURCE_DIR}/src/)
include_directories(${BITMEX_INCLUDE})
include_directories(${GTEST_INCLUDE_DIRS})

#set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
set (CMAKE_CXX_FLAGS "-std=c++11")


target_link_libraries(test_trading_funcs gtest pthread cpprest)


install(TARGETS
            test_trading_funcs
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
