# strategy container dockerfile definiton

FROM alpine:3.14

RUN apk --no-cache add ca-certificates 

# add toolchain
RUN apk add \ 
      g++ \
      gcc \
      cmake \
      git \
      make \
      curl \
      unzip

# install thirdparty libs
RUN apk add \
  gtest-dev \
  boost-dev \
  openssl-dev \
  zlib-dev

# install aws-cli
RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64-2.0.30.zip" -o "awscliv2.zip"
RUN unzip awscliv2.zip
RUN ./aws/install
RUN rm awscliv2.zip aws -r

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
        ../ \
    && make install -j3

# Install benchmark
RUN git clone https://github.com/google/benchmark.git /usr/src/benchmark
RUN mkdir /usr/src/benchmark/build \
    && /bin/sh -c 'cd /usr/src/benchmark/build \
    && cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on \
        -DCMAKE_BUILD_TYPE=Release /usr/src/benchmark \
    && make install -j3 '

# configure tradingo build area
WORKDIR /usr/src/tradingo

ADD thirdparty thirdparty
ADD src src
ADD test test
ADD thirdparty thirdparty
ADD app app
ADD CMakeLists.txt ./CMakeLists.txt
ADD cmake cmake

WORKDIR /usr/src/tradingo/build

# build tradingo
RUN cmake -DCPPREST_ROOT=/usr/local \
        -DREPLAY_MODE=1 \
        -DBOOST_ASIO_DISABLE_CONCEPTS=1 \
        -DCMAKE_INSTALL_PREFIX=/usr/local/tradingo  \
        -Wno-dev ../ \
    && make install -j3

ENV USER=tradingo
ENV UID=12345
ENV GID=23456

RUN addgroup -g $GID tradingo
RUN adduser \
    --disabled-password \
    --gecos "" \
    --home "$(pwd)" \
    --ingroup "$USER" \
    --no-create-home \
    --uid "$UID" \
    "$USER"

RUN install -d -m 0755 -o tradingo -g tradingo /data/tickRecorder/{storage,log}/
RUN install -d -m 0755 -o tradingo -g tradingo /data/replays/{storage,log}
RUN install -d -m 0755 -o tradingo -g tradingo /data/benchmarks/{storage,log}

RUN ["/usr/local/scripts/replay.sh 2021-09-01"]