#!/bin/sh

set -e

readonly version="3.19.4"

case "$( uname -s )" in
    Linux)
        shatool="sha256sum"
        sha256sum="ff23e1f53c53e8ef1fa87568345031d86c504e53efb52fa487db0b8e0ee4d3ff"
        platform="Linux"
        arch="x86_64"
        ;;
    Darwin)
        shatool="shasum -a 256"
        sha256sum="eb1f52996632c1e71a1051c9e2c30cc8df869fb5a213b1a0d3b202744c6c5758"
        platform="macos"
        arch="universal"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform
readonly arch

readonly filename="cmake-$version-$platform-$arch"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > cmake.sha256sum
curl -OL "https://github.com/Kitware/CMake/releases/download/v$version/$tarball"
$shatool --check cmake.sha256sum
tar xf "$tarball"
mv "$filename" cmake

if [ "$( uname -s )" = "Darwin" ]; then
    ln -s CMake.app/Contents/bin cmake/bin
fi
