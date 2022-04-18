# strategy container dockerfile definiton

FROM alpine:3.12 as builder

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

ARG make_flags
RUN echo $make_flags
ENV GNUMAKEFLAGS=${make_flags}



WORKDIR /usr/src

# install cpprest
RUN git clone https://github.com/Microsoft/cpprestsdk.git casablanca

RUN cd casablanca \ 
    && git submodule update --init

RUN cd casablanca \
    && mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_CXX_FLAGS="-Wno-error=format-truncation" \
        -DCMAKE_INSTALL_PREFIX=${install_base}/cpprest \
        ../ \
    && make install -j3

# Install benchmark
RUN git clone https://github.com/google/benchmark.git /usr/src/benchmark
RUN cd benchmark \
    && mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on \
        -DCMAKE_INSTALL_PREFIX=${install_base}/benchmark \
        ../ \
    && make install -j3
