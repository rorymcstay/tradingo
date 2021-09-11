add_executable(replayTradingo
        test_main.cpp)
if(${REPLAY_MODE})
    message(STATUS "Replay mode is active")
    # REPLAY_MODE changes the following
    # TestEnv::replay(quote, trade) will
    # 1. manipulate the Signal class so that it wakes up on the market time
    # 2. play over the files as fast as possible.
    # with replay mode switched off, it will only dispatch events when the amount of time
    # is passed in the market is equal to the amount of actual time passed. This is to mimick
    # realtime trading as close as possible.
    target_compile_definitions(replayTradingo PUBLIC REPLAY_MODE)
endif(${REPLAY_MODE})

target_link_libraries(replayTradingo
        test_trading_funcs
        ${GTEST_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        CppRestSwaggerClient
        boost_system
        pthread
        boost_program_options
        )
install(TARGETS
        replayTradingo
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        )
