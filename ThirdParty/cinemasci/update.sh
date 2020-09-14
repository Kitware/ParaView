#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="cinemasci"
readonly ownership="Cinemasci Python Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/cinemasci/paraview/tpl"
readonly repo="https://github.com/cinemascience/cinemasci.git"
readonly tag="0edef15bf47fe16a65c59093d56d79afc6a2fa27"

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
