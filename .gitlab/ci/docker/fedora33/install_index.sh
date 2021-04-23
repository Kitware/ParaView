#!/bin/sh

set -e

readonly version="5.9.20210503"
readonly tarball="nvidia-index-libs-${version}-linux.tar.bz2"
readonly sha256sum="53b690967c5940a15b82900baa572b0641b043ffe33fd8790fe9df1f161e0c06"

readonly index_root="$HOME/index"

mkdir -p "$index_root"
cd "$index_root"

echo "$sha256sum  $tarball" > index.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check index.sha256sum
tar -C "/usr/local" --strip-components=1 -xf "$tarball"

cd

rm -rf "$index_root"
