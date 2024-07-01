#!/bin/bash
set -xeu -o pipefail

UBUNTU_VERSION=${1:-22.04}
BUILD_DIR="/build"

SCRIPT_DIR=$(dirname $(realpath $0))

[ -r /.dockerenv ] || {
    exec docker run -u root --entrypoint=$BUILD_DIR/$(basename $0) --rm -v $SCRIPT_DIR:$BUILD_DIR public.ecr.aws/lts/ubuntu:$UBUNTU_VERSION
}

apt-get update && apt-get install  -y make g++ libboost-dev libssl-dev libboost-system-dev libboost-thread-dev

mkdir /tmp/build
cp -r $BUILD_DIR/* /tmp/build
cd /tmp/build && make

setpriv $(stat -c "--reuid=%u --regid=%g" $0) --clear-groups cp /tmp/build/chat_server $BUILD_DIR/
