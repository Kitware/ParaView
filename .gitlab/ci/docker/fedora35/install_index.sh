#!/bin/sh

set -e

readonly version="6.0.0.20250604"
readonly tarball="nvidia-index-libs-${version}-linux.tar.bz2"
readonly sha256sum="a24dfe3d76e317bf127be25597b4340a9d2b66f3ff6897a310ff3d8e16a459dc"

readonly index_root="$HOME/index"

mkdir -p "$index_root"
cd "$index_root"

echo "$sha256sum  $tarball" > index.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check index.sha256sum
tar -C "/usr/local" --strip-components=1 -xf "$tarball"

cd

rm -rf "$index_root"
