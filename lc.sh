#!/bin/bash
sudo adduser $USER video
sudo apt-get install git
sudo apt install cmake build-essential
git clone https://github.com/circinusX1/lili
sudo apt-get install libv4l-dev
sudo apt install libjpeg-dev
sudo apt-get install libavcodec-dev libavformat-dev # (optional with code changes)
sudo apt-get install v4l-utils # (optional to adjust luminosity)
cd liveimage
rm bin
mkdir bin
cmake .
make
cp ./liveimage.konf bin
cp ./liveimage bin
echo "edit and configure liveimage.konf"
echo "Thx"

