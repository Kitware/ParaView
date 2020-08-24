#!/bin/sh

set -e

readonly version="3.17.4"
readonly sha256sum="126cc8356907913787d4ff35237ae1854c09b927a35dbe5270dd571ae224bdd3"
readonly filename="cmake-$version-Linux-x86_64"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
sha256sum --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" "cmake"
