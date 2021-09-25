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

message("Configuring config files")
configure_file(etc/config/tradingo.cfg etc/config/tradingo.cfg @ONLY)
configure_file(etc/config/tickRecorder.cfg etc/config/tickRecorder.cfg @ONLY)
configure_file(etc/tickrecorder.service  etc/tickrecorder.service @ONLY)
configure_file(etc/tradingo.service etc/tradingo.service @ONLY)
configure_file(scripts/profile.env scripts/profile.env @ONLY)

install(TARGETS
           tickRecorder
           tradingo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
	COMPONENT app)

install(FILES
	etc/config/tickRecorder.cfg
	etc/config/tradingo.cfg
    etc/config/replayTradingo.cfg
	DESTINATION etc/config COMPONENT app-config)

install(FILES
    scripts/tickrecorder.sh
    scripts/profile.env
    scripts/sync_with_s3.sh
    scripts/replay.sh
    DESTINATION scripts COMPONENT app-scripts)

if(${INSTALL_SERVICE})
    message(STATUS "Installing systemctl service files")
    message(STATUS "Run: ")
    message(STATUS "    $ systemctl daemon-reload ")
    message(STATUS "    $ systemctl start tickrecorder.service")
    message(STATUS "    $ systemctl enable tickrecorder.service")
    install(FILES
        etc/tickrecorder.service
        DESTINATION /etc/systemd/system/ COMPONENT app-config)
endif(${INSTALL_SERVICE})
