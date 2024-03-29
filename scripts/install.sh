#!/bin/bash
#
# Install necessary softwares for Ubuntu.

sudo apt-get update
sudo apt-get install -y \
    gcc-multilib \
    git \
    g++ \
    python3-pip \
    ninja-build \
    unifdef \
    p7zip-full \
    tofrodos \
    gawk \
    software-properties-common

wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
CODENAME=$(lsb_release -c | cut -f2 -d':' | sed 's/\t//')
sudo apt-add-repository -y "deb https://apt.kitware.com/ubuntu/ $CODENAME main"
sudo apt-get -y install cmake

python3 -m pip install --upgrade pip
pip3 install gcovr==4.1
pip3 install --upgrade cmake
