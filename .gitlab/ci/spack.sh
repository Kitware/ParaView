#!/bin/sh
set -e

readonly repo="https://github.com/spack/spack.git"
readonly destdir="$CI_BUILDS_DIR/spack"

rm -rf $destdir
git clone --depth=1 $repo $destdir
