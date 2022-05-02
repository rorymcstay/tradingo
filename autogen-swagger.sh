#!/bin/bash

ROOT_DIR="$(readlink -f $(dirname ${BASH_SOURCE[0]}))"
BUILD_TYPE=${BUILD_TYPE:-Release}
REPLAY_MODE=${REPLAY_MODE:-0}
BUILD_DIR=build.release
if [[ $BUILD_TYPE == 'Debug' ]]; then
    BUILD_DIR=build.debug 
fi

install_base=$ROOT_DIR/install
(
cd $ROOT_DIR/thirdparty/api-connectors/auto-generated/cpprest/ \
    && rm -rf $BUILD_DIR \
    && mkdir $BUILD_DIR \
    && cd $BUILD_DIR \
    && cmake \
        -Wno-dev \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX:-$install_base/swagger} \
        -DCMAKE_PREFIX_PATH="${install_base}/cpprest;" \
        ../ \
    && make install -j6
)
