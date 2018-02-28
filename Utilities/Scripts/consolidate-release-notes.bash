#!/usr/bin/env bash

set -e

usage='usage: consolidate-release-notes.bash <new-release-version> <prev-release-version>'

die() {
    echo "$@" 1>&2; exit 1
}

test "$#" = 2 || die "$usage"

output_file="Documentation/release/$1.md"

files="$(ls Documentation/release/dev/* | grep -v Documentation/release/dev/0-sample-topic.md)"
title="ParaView $1 Release Notes"
underline="$(echo "$title" | sed 's/./=/g')"
echo "$title
$underline

Major changes made since ParaView $2 include the following:
" > $output_file
ls $files | xargs -I% bash -c "cat %; echo" >> $output_file
# Replace section markers with sub-section markers

sed -i 's/# /## /' $output_file

# Remove the last newline character
sed -i -e :a -e '/^\n*$/{$d;N;};/\n$/b' $output_file
rm $files
