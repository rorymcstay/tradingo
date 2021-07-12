FROM gcc:11.1.0

RUN mkdir /usr/src/tradingo
WORKDIR /usr/src/tradingo

ADD thirdparty thirdparty
ADD src src
ADD test test
ADD thirdparty thirdparty
ADD app app
ADD CMakeLists.txt ./CMakeLists.txt
ADD gitVersion.cmake ./gitVersion.cmake

RUN apt-get update && apt-get -y install cmake libcpprest-dev libgtest-dev libboost-program-options-dev

#TODO install packages
RUN mkdir build
RUN cd build && cmake -DCPPREST_ROOT=/usr/ -DCMAKE_INSTALL_PREFIX=/usr/tradingo/  ../
RUN cd build
ENTRYPOINT ["make", "package", "-j3"]
