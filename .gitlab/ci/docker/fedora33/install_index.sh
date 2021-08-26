#!/bin/sh

set -e

readonly version="5.10.20210901"
readonly tarball="nvidia-index-libs-${version}-linux.tar.bz2"
readonly sha256sum="fbf47297e39b0ac2a51e98ce5064d99f29628413c53498b06884e60f01d04482"

readonly index_root="$HOME/index"

mkdir -p "$index_root"
cd "$index_root"

echo "$sha256sum  $tarball" > index.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check index.sha256sum
tar -C "/usr/local" --strip-components=1 -xf "$tarball"

cd

rm -rf "$index_root"
