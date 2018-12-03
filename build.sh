#!/bin/sh
# try build on various platforms and compilers using docker

# exit after first error
set -e

for image in \
	ashutosh108/dev:ubuntu14.04-cmake2.8.12.2-gcc4.8.4 \
	ashutosh108/dev:ubuntu16.04-cmake3.5.1-gcc5.4.0 \
	ashutosh108/dev:ubuntu18.04-cmake3.10.2-gcc7.3.0 \
	ashutosh108/dev:centos7-cmake2.8.12.2-gcc4.8.5 \
	; do

	docker run --rm -v //c/Users/ashutosh/src:/home/dev/src:ro --tmpfs /home/dev/build:exec "$image" su - dev -c "cd build && cmake ../src/sb-sloka-counter && cmake --build ."
done
