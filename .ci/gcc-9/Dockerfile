FROM ubuntu:20.04

# wget needs recommends
RUN apt-get update \
    && apt-get install -y software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test -y \
    && add-apt-repository ppa:beineri/opt-qt-5.15.2-focal -y \
    && apt-get update && apt-get -y install --no-install-recommends build-essential ninja-build qt515base qt515declarative qt515tools qt515multimedia qt515script qt515quickcontrols2 qt515svg liblmdb-dev libgl1-mesa-dev libssl-dev git ccache pkg-config libsecret-1-dev cmake python3-pip \
    && apt-get -y install wget \
    && pip3 install meson \
    && /usr/sbin/update-ccache-symlinks \
		&& apt-get clean

