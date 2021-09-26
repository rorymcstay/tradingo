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
      make \
      curl \
      unzip

# install thirdparty libs
RUN apk add \
  gtest-dev \
  boost-dev \
  openssl-dev \
  zlib-dev

ARG make_flags
RUN echo $make_flags
ENV GNUMAKEFLAGS=${make_flags}

# install aws-cli
RUN curl "https://awscli.amazonaws.com/awscli-exe-linux-x86_64-2.0.30.zip" -o "awscliv2.zip"
RUN unzip awscliv2.zip
RUN ./aws/install --install-dir ${install_base}/aws-cli
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
        -DCMAKE_INSTALL_PREFIX=${install_base}/cpprest \
        ../ \
    && make install -j6

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
    && make install -j6

# build tradingo
ADD . tradingo
RUN cd tradingo \
    && mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DCPPREST_ROOT=${install_base}/cpprest \
        -DREPLAY_MODE=1 \
        -DBOOST_ASIO_DISABLE_CONCEPTS=1 \
        -Wno-dev ../ \
        -DCMAKE_INSTALL_PREFIX=${install_base}/tradingo \
        -DCMAKE_PREFIX_PATH="${install_base}/cpprest;${install_base}/benchmark" \
    && make install -j6

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

# runtime image
FROM alpine:3.14

# aws authentication
ARG aws_secret_access_key
ARG aws_access_key_id
ARG aws_region
ENV AWS_SECRET_ACCESS_KEY ${aws_secret_access_key}
ENV AWS_ACCESS_KEY_ID ${aws_access_key_id}
ENV AWS_REGION ${aws_region}

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
COPY --from=builder /usr/local/* /usr/local/
COPY --from=builder ${install_base}/cpprest/* /usr/local/
COPY --from=builder ${install_base}/benchmark/* /usr/local/
COPY --from=builder ${install_base}/aws-cli/* /usr/local/
COPY --from=builder /data/ /data/

#RUN ["/usr/local/scripts/replay.sh 2021-09-01"]
