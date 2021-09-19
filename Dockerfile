FROM ubuntu:20.04 AS builder

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata

RUN apt-get update \
  && apt-get install -y ssh \
      build-essential \
      gcc \
      g++ \
      gdb \
      clang \
      cmake \
      python \
      less \
      git \
  && apt-get clean

RUN apt-get install -y \
  libcpprest-dev \
  libgtest-dev \
  libboost-all-dev \
  rpm


# Install benchmark
RUN mkdir -p /usr/src/
WORKDIR /usr/src

RUN git clone https://github.com/google/benchmark.git /usr/src/benchmark
RUN mkdir /usr/src/benchmark/build \
    && /bin/sh -c 'cd /usr/src/benchmark/build && \
        cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on \
        -DCMAKE_BUILD_TYPE=Release /usr/src/benchmark \
    && make install -j3 '

RUN mkdir /usr/src/tradingo
WORKDIR /usr/src/tradingo

ADD thirdparty thirdparty
ADD src src
ADD test test
ADD thirdparty thirdparty
ADD app app
ADD CMakeLists.txt ./CMakeLists.txt
ADD gitVersion.cmake ./gitVersion.cmake


RUN mkdir build
RUN cd build && cmake -DCPPREST_ROOT=/usr/ -DCMAKE_INSTALL_PREFIX=/usr/tradingo/ -DREPLAY_MODE=1 ../
RUN cd build && make install -j3

FROM alpine-3.14.2

ARG AWS_ACCESS_KEY
ARG AWS_SECRET_KEY
COPY --from=builder /usr/tradingo/ /usr/
RUN apk --no-cache add ca-certificates
WORKDIR /root/
COPY --from=builder /go/src/github.com/alexellis/href-counter/app ./

RUN useradd -m tradingo
RUN install -d -m 0755 -o tradingo -g tradingo /data/tickRecorder/{storage,log}/
RUN install -d -m 0755 -o tradingo -g tradingo /data/replays/{storage,log}
RUN install -d -m 0755 -o tradingo -g tradingo /data/benchmarks/{storage,log}

RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64-2.0.30.zip" -o "awscliv2.zip"
RUN unzip awscliv2.zip
RUN sudo ./aws/install
RUN rm awscliv2.zip aws -r


RUN ["/usr/local/scripts/replay.sh 2021-09-01"]