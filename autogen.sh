#!/bin/sh

rm -r build
rm compile_commands.json

BUILD_TYPE=${BUILD_TYPE:-Release}
REPLAY_MODE=${REPLAY_MODE:-0}

BUILD_DIR=$(if [[ $BUILD_TYPE == 'Release' ]]; then echo build.release; else echo build.debug; fi)

mkdir $BUILD_DIR 

(
cd $BUILD_DIR

cmake -DCPPREST_ROOT=/usr/ \
    -DCMAKE_COMPILE_COMMANDS=1 \
    -DCPPREST_ROOT=${CPPREST_ROOT:-/usr/}
    "@" \
    ../
)

ln -s build/compile_commands.json compile_commands.json

(
cd $BUILD_DIR && make install -j6
)
