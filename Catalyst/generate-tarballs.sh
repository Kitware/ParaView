#!/usr/bin/env bash

# A quick script to generate source tarball for all editions.

usage="$0 <suffix>"
# A handy function for handling errors.
die () {
    echo >&2 "$@"
    exit 1
}

[ "$#" -lt 1 ] && \
    die "$usage"

readlink_cmd="readlink"
if [[ $OSTYPE == darwin* ]]; then
  # Use greadlink on the mac
  readlink_cmd="greadlink"
fi

scriptdir="$0"
scriptdir="$( dirname "$scriptdir" )"
scriptdir="$( "${readlink_cmd}" -f "$scriptdir" )"

suffix="$1"
src_output="$( pwd )"

generate_edition () {
  local parts="$@"
  cmd=""
  editionsuffix=""
  for part in $parts; do
    cmd="$cmd --input $part"
    editionsuffix="$editionsuffix-$part"
  done
  $scriptdir/test-catalyst.sh $(pwd)/Catalyst-$suffix$editionsuffix \
    $scriptdir/tmp \
    --no-configure --no-build --no-test --remove-dirs $cmd
  tar zcf Catalyst-$suffix$editionsuffix.tar.gz Catalyst-$suffix$editionsuffix
  mv Catalyst-$suffix$editionsuffix.tar.gz ../
}

rm -rf $src_output/__tmp__
mkdir -p $src_output/__tmp__
cd $src_output/__tmp__
generate_edition "HostTools"
generate_edition "Base"
generate_edition "Base" "Essentials"
generate_edition "Base" "Essentials" "Extras"
generate_edition "Base" "Essentials" "Extras" "Rendering-Base"

generate_edition "Base" "Enable-Python"
generate_edition "Base" "Enable-Python" "Essentials"
generate_edition "Base" "Enable-Python" "Essentials" "Extras"
generate_edition "Base" "Enable-Python" "Essentials" "Extras" "Rendering-Base"
cd ../
rm -rf $src_output/__tmp__
