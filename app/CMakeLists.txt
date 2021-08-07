link_directories(/usr/local/lib)

add_executable(tickRecorder bin/tickRecorder.cpp etc/config/tickRecorder.cfg)
add_executable(tradingo bin/tradingo.cpp etc/config/tradingo.cfg)


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

configure_file(etc/config/tradingo.cfg etc/config/tradingo.cfg @ONLY)
configure_file(etc/config/tickRecorder.cfg etc/config/tickRecorder.cfg @ONLY)
configure_file(etc/tickrecorder.service  etc/tickrecorder.service @ONLY)
configure_file(etc/tradingo.service etc/tradingo.service @ONLY)

install(TARGETS
           tickRecorder
           tradingo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
	COMPONENT app)

install(FILES
	etc/config/tickRecorder.cfg
	etc/config/tradingo.cfg
	DESTINATION etc/config COMPONENT app-config)

install(FILES
    scripts/tickrecorder.sh
    DESTINATION scripts COMPONENT app-scripts)
