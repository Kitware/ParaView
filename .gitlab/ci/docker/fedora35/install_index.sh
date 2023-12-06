#!/bin/sh

set -e

readonly version="5.12.0.20231121"
readonly tarball="nvidia-index-libs-${version}-linux.tar.bz2"
readonly sha256sum="f82a859482f774f5639be9ddb1c45d1f117a661cab303e8edae93d357729afbe"

readonly index_root="$HOME/index"

mkdir -p "$index_root"
cd "$index_root"

echo "$sha256sum  $tarball" > index.sha256sum
curl -OL "https://www.paraview.org/files/dependencies/$tarball"
sha256sum --check index.sha256sum
tar -C "/usr/local" --strip-components=1 -xf "$tarball"

cd

rm -rf "$index_root"
