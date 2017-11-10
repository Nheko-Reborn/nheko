#!/usr/bin/env bash

sudo add-apt-repository -y ppa:beineri/opt-qt592-trusty
sudo add-apt-repository -y ppa:george-edison55/cmake-3.x
sudo apt-get update -qq
sudo apt-get install -qq -y qt59base qt59tools cmake liblmdb-dev
