# CLion remote docker environment (How to build docker container, run and stop it)
#
# Build and run:
#   docker build -t clion/remote-cpp-env:0.5 -f TickRecorder.dockerfile.remote-cpp-env .
#   docker run -d --cap-add sys_ptrace -p127.0.0.1:2222:22 --name clion_remote_env clion/remote-cpp-env:0.5
#   ssh-keygen -f "$HOME/.ssh/known_hosts" -R "[localhost]:2222"
#
# stop:
#   docker stop clion_remote_env
# 
# ssh credentials (test user):
#   user@password 

FROM ubuntu:20.04

RUN DEBIAN_FRONTEND="noninteractive" apt-get update && apt-get -y install tzdata

RUN apt-get update \
  && apt-get install -y ssh \
      build-essential \
      gcc \
      g++ \
      gdb \
      clang \
      cmake \
      rsync \
      tar \
      python \
      less \
      git \
  && apt-get clean

RUN apt-get install -y \
  libcpprest-dev \
  libgtest-dev \
  libboost-all-dev \
  rpm

RUN ( \
    echo 'LogLevel INFO'; \
    echo 'PermitRootLogin yes'; \
    echo 'PasswordAuthentication yes'; \
    echo 'Subsystem sftp /usr/lib/openssh/sftp-server'; \
  ) > /etc/ssh/sshd_config_test_clion \
  && mkdir /run/sshd


# Install benchmark
RUN mkdir -p /usr/src/
WORKDIR /usr/src

RUN git clone https://github.com/google/benchmark.git /usr/src/benchmark
RUN mkdir /usr/src/benchmark/build \
    && /bin/sh -c 'cd /usr/src/benchmark/build && \
        cmake -DBENCHMARK_DOWNLOAD_DEPENDENCIES=on \
        -DCMAKE_BUILD_TYPE=Release /usr/src/benchmark \
    && make install -j3 '

RUN useradd -m tradingo \
  && yes password | passwd tradingo

RUN usermod -s /bin/bash tradingo

CMD ["/usr/sbin/sshd", "-D", "-e", "-f", "/etc/ssh/sshd_config_test_clion"]