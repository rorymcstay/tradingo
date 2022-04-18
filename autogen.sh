#!/bin/bash

ROOT_DIR="$(readlink -f $(dirname ${BASH_SOURCE[0]}))"
BUILD_TYPE=${BUILD_TYPE:-Release}
REPLAY_MODE=${REPLAY_MODE:-0}
BUILD_DIR=$ROOT_DIR/build.release
if [[ $BUILD_TYPE == 'Debug' ]]; then
    BUILD_DIR=$ROOT_DIR/build.debug 
fi

rm -r $BUILD_DIR &> /dev/null
rm compile_commands.json &> /dev/null

mkdir $BUILD_DIR 

(
cd $BUILD_DIR

cmake -DCPPREST_ROOT=/usr/ \
    -Wno-dev \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
    -DCPPREST_ROOT=${CPPREST_ROOT:-/usr/} \
    -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX:-$ROOT_DIR/install} \
    -DWITH_REGRESSION_TEST=1 \
    "$@" \
    $ROOT_DIR
)

ln -s $BUILD_DIR/compile_commands.json $ROOT_DIR/compile_commands.json
