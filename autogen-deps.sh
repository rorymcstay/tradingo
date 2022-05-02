#!/bin/bash

ROOT_DIR="$(readlink -f $(dirname ${BASH_SOURCE[0]}))"
BUILD_TYPE=${BUILD_TYPE:-Release}
REPLAY_MODE=${REPLAY_MODE:-0}
BUILD_DIR=$ROOT_DIR/build.release

install_base=$ROOT_DIR/install

# install cpprest

git submodule update --init --recursive

(
cd $ROOT_DIR/thirdparty/cpprestsdk \
    && rm -r build.release \
    && mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-Wno-error=format-truncation" \
        -DCMAKE_INSTALL_PREFIX=${install_base}/cpprest \
        ../ \
    && make install -j6
)
(
cd $ROOT_DIR/thirdparty/benchmark \
    && rm -r build.release \
    && mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on \
        -DCMAKE_INSTALL_PREFIX=${install_base}/benchmark \
        ../ \
    && make install -j6
)
(
cd $ROOT_DIR/thirdparty/aws-sdk-cpp \
    && rm -r build.release \
    && mkdir build.release \
    && cd build.release \
    && cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=${install_base}/aws/ \
        -DBUILD_ONLY=s3 \
        ../ \
    && make install -j6
)
