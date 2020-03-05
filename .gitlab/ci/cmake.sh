#!/bin/sh

set -e

readonly version="3.15.6"
readonly sha256sum="05510247c4ef28d011b5787972159b05456c49630781deba2bfb71fcf81a1b40"
readonly filename="cmake-$version-Linux-x86_64"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
sha256sum --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" "cmake"
