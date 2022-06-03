<<<<<<< HEAD
FROM rmcstay95/tradingo-base:63c299a-dirty as builder
=======
FROM rmcstay95/tradingo-base:2da4186-dirty as builder
>>>>>>> 653db3c7f44e9f981283f8663971d08e34fb1ebc

# build tradingo
# TODO break this up into compilation of targets one at a time
# and then install only at the end. This is in order to take
# advantage of layer caching if possible. And to only build what
# image needs
ARG install_base=/usr/from-src/

ARG CMAKE_BUILD_TYPE=RelWithDebInfo

WORKDIR /usr/src/tradingo
ADD . .
RUN git submodule update --init
RUN mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
        -DCPPREST_ROOT=${install_base}/cpprest \
        -DREPLAY_MODE=1 \
        -DBOOST_ASIO_DISABLE_CONCEPTS=1 \
        -Wno-dev \
        -DCMAKE_INSTALL_PREFIX=${install_base}/tradingo \
        -DCMAKE_PREFIX_PATH="${install_base}/cpprest;${install_base}/benchmark;${install_base}/aws;${install_base}/swagger" \
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
FROM alpine:3.12

# install thirdpary libraries
RUN apk add \
  gtest-dev \
  boost-dev \
  openssl-dev \
  zlib-dev \
  libstdc++

RUN apk add gettext \
    curl \
    unzip \
    bash

# aws authentication
ARG aws_secret_access_key
ARG aws_access_key_id
ARG aws_region
ENV AWS_SECRET_ACCESS_KEY ${aws_secret_access_key}
ENV AWS_ACCESS_KEY_ID ${aws_access_key_id}
ENV AWS_REGION ${aws_region}


# install aws-cli

# install glibc compatibility for alpine
ENV GLIBC_VER=2.31-r0
RUN apk --no-cache add \
        binutils \
        gdb \
        curl \
        musl-dbg \
    && curl -sL https://alpine-pkgs.sgerrand.com/sgerrand.rsa.pub -o /etc/apk/keys/sgerrand.rsa.pub \
    && curl -sLO https://github.com/sgerrand/alpine-pkg-glibc/releases/download/${GLIBC_VER}/glibc-${GLIBC_VER}.apk \
    && curl -sLO https://github.com/sgerrand/alpine-pkg-glibc/releases/download/${GLIBC_VER}/glibc-bin-${GLIBC_VER}.apk \
    && curl -sLO https://github.com/sgerrand/alpine-pkg-glibc/releases/download/${GLIBC_VER}/glibc-i18n-${GLIBC_VER}.apk \
    && apk add --no-cache \
        glibc-${GLIBC_VER}.apk \
        glibc-bin-${GLIBC_VER}.apk \
        glibc-i18n-${GLIBC_VER}.apk \
    && /usr/glibc-compat/bin/localedef -i en_US -f UTF-8 en_US.UTF-8 \
    && curl -sL https://awscli.amazonaws.com/awscli-exe-linux-x86_64.zip -o awscliv2.zip \
    && unzip awscliv2.zip \
    && aws/install \
    && rm -rf \
        awscliv2.zip \
        aws \
        /usr/local/aws-cli/v2/*/dist/aws_completer \
        /usr/local/aws-cli/v2/*/dist/awscli/data/ac.index \
        /usr/local/aws-cli/v2/*/dist/awscli/examples \
        glibc-*.apk \
    && apk --no-cache del \
        binutils \
        curl \
    && rm -rf /var/cache/apk/*


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

ENV TESTDATA_LOCATION=/usr/local/etc/test/data

ARG install_base=/usr/from-src/
COPY --from=builder ${install_base}/cpprest/lib64 /usr/local/lib
COPY --from=builder ${install_base}/cpprest/include /usr/local/include
COPY --from=builder ${install_base}/swagger/lib /usr/local/lib
COPY --from=builder ${install_base}/swagger/include /usr/local/include
COPY --from=builder ${install_base}/benchmark/lib /usr/local/lib
COPY --from=builder ${install_base}/benchmark/lib64 /usr/local/lib
COPY --from=builder ${install_base}/benchmark/include /usr/local/include
COPY --from=builder ${install_base}/aws/include /usr/local/include
COPY --from=builder ${install_base}/aws/lib /usr/local/include
COPY --from=builder ${install_base}/tradingo/lib /usr/local/lib
COPY --from=builder ${install_base}/tradingo/bin /usr/local/bin
COPY --from=builder ${install_base}/tradingo/etc /usr/local/etc
COPY --from=builder ${install_base}/tradingo/scripts /usr/local/scripts
COPY --from=builder /data/ /data/
