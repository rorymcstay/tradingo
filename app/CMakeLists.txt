add_executable(tickRecorder tickRecorder.cpp)

target_link_libraries(tickRecorder trading_funcs)
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${BITMEX_INCLUDE})

install(TARGETS
           tickRecorder
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
