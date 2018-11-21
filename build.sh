#!/bin/sh
# try build on various platforms and compilers using docker

# ubuntu 14.04 (LTS 2014-2019), CMake 2.8.12.2, g++ 4.8.4
docker run --rm -v //c/Users/ashutosh/src:/home/dev/src:ro --tmpfs /home/dev/build:exec ashutosh108/dev:cmake2.8-gcc4.8 su - dev -c "cd build && cmake ../src/sb-sloka-counter && cmake --build ."
