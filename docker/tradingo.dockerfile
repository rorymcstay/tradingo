
FROM rmcstay95/tradingo-base:0ee37ab-dirty as builder

# build tradingo
ARG install_base=/usr/from-src/
WORKDIR /usr/src/tradingo
ADD . .
RUN git submodule update --init
RUN mkdir build.release \
    && cd build.release \
    && cmake -DCMAKE_BUILD_TYPE=Release \
        -DCPPREST_ROOT=${install_base}/cpprest \
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

RUN install -d -m 0755 -o tradingo -g tradingo /data/tradingo/storage
RUN install -d -m 0755 -o tradingo -g tradingo /data/tradingo/log

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
    unzip \
    bash

# install aws-cli
# install glibc compatibility for alpine
ENV GLIBC_VER=2.31-r0
RUN apk --no-cache add \
        binutils \
        curl \
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

ARG install_base=/usr/from-src/
COPY --from=builder ${install_base}/cpprest/lib /usr/local/lib
COPY --from=builder ${install_base}/cpprest/include /usr/local/include
COPY --from=builder ${install_base}/benchmark/lib /usr/local/lib
COPY --from=builder ${install_base}/benchmark/include /usr/local/include
COPY --from=builder ${install_base}/tradingo/lib /usr/local/lib
COPY --from=builder ${install_base}/tradingo/bin /usr/local/bin
COPY --from=builder ${install_base}/tradingo/etc /usr/local/etc
COPY --from=builder ${install_base}/tradingo/scripts /usr/local/scripts
COPY --from=builder /data/ /data/
