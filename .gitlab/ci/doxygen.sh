#!/bin/sh

set -e

readonly version="1.8.13"
readonly sha256sum="1b3ceb3708c5099d51341cd4ac63893f05c736388f12fb99958081fc832a3b3e"

readonly target="doxygen-$version"
readonly tarball="$target.linux.bin.tar.gz"

cd .gitlab

echo "$sha256sum $tarball" > doxygen.sha256
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check doxygen.sha256

cmake/bin/cmake -E tar xf "$tarball"
mv "$target" doxygen
