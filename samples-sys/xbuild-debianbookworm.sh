#!/bin/bash

# cpspg: cross-build on Linux for Debian-bookworm

IMAGE_NAME=cpspg-debianbookworm-builder
CONTAINER_NAME=cpspg_debianbookworm_build

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
 gcc g++
EOF
	fi

	# Create builder container
	podman create --attach --tty \
	 -v `pwd`/..:/src \
	 --name $CONTAINER_NAME \
	 $IMAGE_NAME \
	 bash -c 'cd /src/cpspg && source ./build_linux.sh'
fi

# Prepare build script
cat >build_linux.sh <<EOF
set -xe

mkdir -p samples/_linux
cd samples/_linux
make -j8 \
 -f ../Makefile \
 SRC_DIR=.. \
 $@
bash ../unix-run-all.sh
EOF

# Build inside the container
podman start --attach $CONTAINER_NAME
