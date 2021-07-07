link_directories(/usr/local/lib)

add_executable(tickRecorder tickRecorder.cpp)
add_executable(tradingo tradingo.cpp)
# add_executable(tradingoLoader tradingoLoader.cpp)


target_link_libraries(tickRecorder
        trading_funcs
        ${OPENSSL_CRYPTO_LIBRARY}
        boost_program_options)
target_link_libraries(tradingo
        trading_funcs
        ${OPENSSL_CRYPTO_LIBRARY}
        boost_program_options)

#target_link_libraries(tradingoLoader
#        trading_funcs
#        ${OPENSSL_CRYPTO_LIBRARY}
#        boost_program_options)

target_link_libraries(tradingo trading_funcs)

include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${BITMEX_INCLUDE})
include_directories(${Boost_INCLUDE_DIR})
include_directories(${OPENSSL_INCLUDE_DIR})
include_directories(${CPPLOGGER_INCLUDES})

install(TARGETS
           tickRecorder
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
