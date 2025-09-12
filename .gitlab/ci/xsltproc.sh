#!/bin/sh

set -e

readonly version="1.1.42"
readonly full_version="$version-20250904.1"

case "$( uname -s )-$( uname -m )" in
    Darwin-arm64)
        shatool="shasum -a 256"
        sha256sum="c2c71ac1358687a7aed51b5b97965154dd2967758d5d291a94a06fa9ba17cd52"
        platform="Darwin-arm64"
        ;;
    Darwin-x86_64)
        shatool="shasum -a 256"
        sha256sum="f2b8459065f25ef9a51a93c6a353b9cbb8cf90aa559e60b60349721f80e03454"
        platform="Darwin-x86_64"
        ;;
    *)
        echo "Unrecognized platform $( uname -s )-$( uname -m )"
        exit 1
        ;;
esac
readonly shatool
readonly sha256sum
readonly platform

readonly filename="xsltproc-$version-$platform"
readonly tarball="$filename.tar.gz"

cd .gitlab

echo "$sha256sum  $tarball" > xsltproc.sha256sum
curl -OL "https://gitlab.kitware.com/api/v4/projects/6955/packages/generic/xsltproc/v$full_version/$tarball"
$shatool --check xsltproc.sha256sum
./cmake/bin/cmake -E tar xf "$tarball"
mv "$filename" xsltproc
