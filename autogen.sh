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
cd $ROOT_DIR \
    && rm -rf $BUILD_DIR \
    && mkdir $BUILD_DIR \
    && cd $BUILD_DIR \
    && cmake \
        -Wno-dev \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=1 \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX:-$install_base/tradingo} \
        -DWITH_REGRESSION_TEST=1 \
        -DPACKAGE_INSTALLS="${install_base}" \
        -DCMAKE_MODULE_PATH="${install_base}/swagger/lib/;${install_base}/aws/lib/cmake/AWSSDK/" \
        -DPACKAGE_INSTALLS="$install_base"
        "$@" \
        ../ \
    && make install -j6
)
