#!/bin/sh

set -e

readonly VERSION="1.8.13"
readonly SHA256SUM="1b3ceb3708c5099d51341cd4ac63893f05c736388f12fb99958081fc832a3b3e"

readonly TARGET="doxygen-$VERSION"
readonly TARBALL="$TARGET.linux.bin.tar.gz"

cd .gitlab

echo "$SHA256SUM $TARBALL" > doxygen.sha256
curl -OL "https://sourceforge.net/projects/doxygen/files/rel-$VERSION/$TARBALL"
sha256sum --check doxygen.sha256

cmake/bin/cmake -E tar xf "$TARBALL"
mv "$TARGET" doxygen
