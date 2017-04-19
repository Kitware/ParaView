#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="pygments"
readonly ownership="pygments Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/$name/vtk$name"
readonly repo="https://gitlab.kitware.com/third-party/pygments.git"
readonly tag="for/paraview"

readonly paths="
AUTHORS
CHANGES
LICENSE
pygmentize

pygments/*.py
pygments/filters
pygments/formatters
pygments/lexers/__init__.py
pygments/lexers/_mapping.py
pygments/lexers/agile.py
pygments/lexers/special.py
pygments/lexers/python.py
pygments/styles
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
