FROM ubuntu:trusty

RUN \
    apt-get update -qq && \
    apt-get install -y software-properties-common && \
    add-apt-repository -y ppa:beineri/opt-qt-5.10.1-trusty && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    add-apt-repository -y ppa:chris-lea/libsodium && \
    apt-get update -qq && \
    apt-get install -y \
        qt510base qt510tools qt510svg qt510multimedia \
        gcc-5 g++-5

RUN \
    apt-get install -y \
        make \
        pkg-config \
        ninja-build \
        libsodium-dev \
        liblmdb-dev \
        libssl-dev \
        mesa-common-dev \
        wget \
        fuse \
        git

RUN \
    wget https://cmake.org/files/v3.12/cmake-3.12.2-Linux-x86_64.sh && \
    sudo sh cmake-3.12.2-Linux-x86_64.sh  --skip-license  --prefix=/usr/local

RUN \
    update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-5 10 && \
    update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 10 && \
    update-alternatives --set gcc "/usr/bin/gcc-5" && \
    update-alternatives --set g++ "/usr/bin/g++-5"

ENV PATH=/opt/qt510/bin:$PATH

RUN mkdir /build

WORKDIR /build
