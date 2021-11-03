#!/bin/bash


BUILD_TYPE=${BUILD_TYPE:-Release}
REPLAY_MODE=${REPLAY_MODE:-0}

BUILD_DIR=$(if [[ $BUILD_TYPE == 'Release' ]]; then echo build.release; else echo build.debug; fi)

if [[ -d $BUILD_DIR ]] ; then
    rm -r $BUILD_DIR
fi

mkdir $BUILD_DIR 

(
cd $BUILD_DIR

cmake -DCPPREST_ROOT=/usr/ \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCPPREST_ROOT=${CPPREST_ROOT:-/usr/} \
    "$@" \
    ../
)

ln -s $BUILD_DIR/compile_commands.json compile_commands.json
