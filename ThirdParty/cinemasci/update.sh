#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="cinemasci"
readonly ownership="Cinemasci Python Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/cinemasci/paraview/tpl"
readonly repo="https://github.com/cinemascience/cinemasci.git"
readonly tag="5e6c7e96083474ed52de30459602dd79ac600aa1"

readonly paths="
.gitignore
license.md
cinemasci/
"

extract_source () {
    git_archive

    echo "* -whitespace" > "$extractdir/$name-reduced/.gitattributes"
    echo >> "$extractdir/$name-reduced/cinemasci/cview/cinema/lib/d3.v4.min.js"
    echo >> "$extractdir/$name-reduced/cinemasci/cview/cinema/view/1.2/css/range-css.css"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
