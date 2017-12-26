FROM ubuntu:xenial

RUN \
    apt-get update -qq && \
    apt-get install -y software-properties-common

RUN \
    add-apt-repository -y ppa:beineri/opt-qt592-xenial && \
    apt-get update -qq && \
    apt-get install -y \
        qt59base \
        qt59tools \
        qt59multimedia

RUN \
    add-apt-repository ppa:ubuntu-toolchain-r/test && \
    apt-add-repository "deb http://apt.llvm.org/xenial/ llvm-toolchain-xenial-5.0 main" && \
    apt-get update -qq && \
    apt-get install -y --allow-unauthenticated \
        gcc-7 \
        g++-7 \
        cmake \
        clang-5.0 \
        clang-format-5.0 \
        liblmdb-dev

RUN apt-get install -y mesa-common-dev wget fuse git

RUN update-alternatives --install \
        /usr/bin/clang-format \
        clang-format \
        /usr/bin/clang-format-5.0 100

RUN apt-get -y install ruby ruby-dev rubygems rpm && \
    gem install --no-ri --no-rdoc fpm

ENV PATH=/opt/qt59/bin:$PATH

RUN mkdir /build

WORKDIR /build
