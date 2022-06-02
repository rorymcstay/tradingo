# strategy container dockerfile definiton

FROM alpine:3.14 as builder

RUN apk --no-cache add ca-certificates 

ARG install_base=/usr/from-src/

# add toolchain
RUN apk add \ 
      g++ \
      gcc \
      cmake \
      git \
      make

# install thirdparty libs
RUN apk add \
  gtest-dev \
  boost-dev \
  openssl-dev \
  zlib-dev

ARG BUILD_TYPE=Release
ARG make_flags
RUN echo $make_flags
ENV GNUMAKEFLAGS=${make_flags}

WORKDIR /usr/src

# install cpprest
RUN git clone https://github.com/Microsoft/cpprestsdk.git cpprestsdk
RUN cd cpprestsdk \ 
    && git submodule update --init \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_CXX_FLAGS="-Wno-error=format-truncation" \
        -DCMAKE_INSTALL_PREFIX=${install_base}/cpprest \
        ../ \
    && make install -j6

# Install benchmark
RUN git clone https://github.com/google/benchmark.git /usr/src/benchmark
RUN cd benchmark \
    && mkdir build \
    && cd build \
    && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on \
        -DCMAKE_INSTALL_PREFIX=${install_base}/benchmark \
        ../ \
    && make install -j6

# Install aws-sdk-cpp
RUN apk add \
        curl-dev \
        libressl-dev \
    && git clone --recurse-submodules https://github.com/aws/aws-sdk-cpp.git /usr/src/aws-sdk-cpp \
    && mkdir /usr/src/aws-sdk-cpp/build \
    && cd /usr/src/aws-sdk-cpp/build \
    && cmake \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=${install_base}/aws/ \
        -DBUILD_ONLY=s3 \
        /usr/src/aws-sdk-cpp/ \
    && make install -j6


RUN git clone https://github.com/rorymcstay/api-connectors.git /usr/src/api-connectors/ \
    && mkdir /usr/src/api-connectors/auto-generated/cpprest/build \
    && cd /usr/src/api-connectors/auto-generated/cpprest/build \
    && cmake \
        -Wno-dev \
        -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
        -DCMAKE_INSTALL_PREFIX=${install_base}/swagger \
        -DCMAKE_PREFIX_PATH="${install_base}/cpprest;" \
        ../ \
    && make install -j6
