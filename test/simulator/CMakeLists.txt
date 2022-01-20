find_package (Threads)
find_package(GTest)

include_directories(${GTEST_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(orderbook
        src/OrderBook.h
        src/Domain.h
        src/ExecWriter.h)
set_target_properties(orderbook PROPERTIES LINKER_LANGUAGE CXX)

add_executable(test_orderbook 
    test/fwk/TestEnv.cpp
    test/fwk/TestDomain.h
    test/main.cpp
)

target_link_libraries(test_orderbook ${GTEST_LIBRARIES} pthread orderbook)

install(TARGETS
    orderbook
    test_orderbook
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)
