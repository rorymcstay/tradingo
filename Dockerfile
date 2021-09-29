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

RUN ls /${install_base}/cpprest/lib/

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

# build tradingo
RUN git clone https://github.com/rorymcstay/tradingo.git /usr/src/tradingo
RUN cd tradingo \ 
    && git submodule update --init
RUN cd tradingo \
    && mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DCPPREST_ROOT=${install_base}/cpprest \
        -DREPLAY_MODE=1 \
        -DBOOST_ASIO_DISABLE_CONCEPTS=1 \
        -Wno-dev \
        -DCMAKE_INSTALL_PREFIX=${install_base}/tradingo \
        -DCMAKE_PREFIX_PATH="${install_base}/cpprest;${install_base}/benchmark" \
        ../ \
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

RUN install -d -m 0755 -o tradingo -g tradingo /data/tickRecorder/storage
RUN install -d -m 0755 -o tradingo -g tradingo /data/tickRecorder/log
RUN install -d -m 0755 -o tradingo -g tradingo /data/replays/storage
RUN install -d -m 0755 -o tradingo -g tradingo /data/replays/log
RUN install -d -m 0755 -o tradingo -g tradingo /data/benchmarks/storage
RUN install -d -m 0755 -o tradingo -g tradingo /data/benchmarks/log

# runtime image
FROM alpine:3.14

# install thirdpary libraries
RUN apk add \
  gtest-dev \
  boost-dev \
  openssl-dev \
  zlib-dev \
  libstdc++

RUN apk add gettext \
    curl \
    unzip

# aws authentication
ARG aws_secret_access_key
ARG aws_access_key_id
ARG aws_region
ENV AWS_SECRET_ACCESS_KEY ${aws_secret_access_key}
ENV AWS_ACCESS_KEY_ID ${aws_access_key_id}
ENV AWS_REGION ${aws_region}


# install aws-cli
RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64-2.0.30.zip" -o "awscliv2.zip"
RUN unzip awscliv2.zip
RUN ./aws/install --install-dir ${install_base}/aws-cli
RUN rm awscliv2.zip aws -r

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
ARG install_base=/usr/from-src/
COPY --from=builder /usr/local/lib /usr/local/lib
COPY --from=builder ${install_base}/cpprest/lib /usr/local/lib
COPY --from=builder ${install_base}/cpprest/include /usr/local/include
COPY --from=builder ${install_base}/benchmark/lib /usr/local/lib
COPY --from=builder ${install_base}/benchmark/include /usr/local/include
COPY --from=builder ${install_base}/tradingo/lib /usr/local/lib
COPY --from=builder ${install_base}/tradingo/bin /usr/local/bin
COPY --from=builder ${install_base}/tradingo/etc /usr/local/etc
COPY --from=builder ${install_base}/tradingo/scripts /usr/local/scripts
COPY --from=builder /data/ /data/

#RUN ["/usr/local/scripts/replay.sh 2021-09-01"]
