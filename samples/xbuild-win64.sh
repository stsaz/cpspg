#!/bin/bash

# cpspg: cross-build on Linux for Windows/AMD64

IMAGE_NAME=cpspg-win64-builder
CONTAINER_NAME=cpspg_win64_build

set -xe

if ! test -d "../cpspg" ; then
	exit 1
fi

if ! podman container exists $CONTAINER_NAME ; then
	if ! podman image exists $IMAGE_NAME ; then

		# Create builder image
		cat <<EOF | podman build -t $IMAGE_NAME -f - .
FROM debian:bookworm-slim
RUN apt update && \
 apt install -y \
  make
RUN apt install -y \
  gcc-mingw-w64-x86-64 g++-mingw-w64-x86-64
EOF
	fi

	# Create builder container
	podman create --attach --tty \
	 -v `pwd`/..:/src \
	 --name $CONTAINER_NAME \
	 $IMAGE_NAME \
	 bash -c 'cd /src/cpspg && source ./build_mingw64.sh'
fi

# Prepare build script
cat >build_mingw64.sh <<EOF
set -xe

mkdir -p samples/_windows
make -j8 \
 -C samples/_windows \
 -f ../Makefile \
 SRC_DIR=.. \
 OS=windows \
 $@
EOF

# Build inside the container
podman start --attach $CONTAINER_NAME
