#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="cinema"
readonly ownership="Cinema Python Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/cinema/paraview/tpl"
readonly repo="https://gitlab.kitware.com/third-party/cinema_python.git"
readonly tag="for/paraview-20200303"

readonly paths="
.gitignore
LICENSE
cinema_python/database/store.py
cinema_python/database/file_store.py
cinema_python/database/raster_wrangler.py
cinema_python/database/oexr_helper.py
cinema_python/database/vti_store.py
cinema_python/database/__init__.py
cinema_python/images/layer_rasters.py
cinema_python/images/lookup_tables.py
cinema_python/images/querymaker.py
cinema_python/images/camera_utils.py
cinema_python/images/__init__.py
cinema_python/images/querymaker_specb.py
cinema_python/images/compositor.py
cinema_python/__init__.py
cinema_python/adaptors/paraview/pv_introspect.py
cinema_python/adaptors/paraview/progress.py
cinema_python/adaptors/paraview/__init__.py
cinema_python/adaptors/paraview/pv_explorers.py
cinema_python/adaptors/paraview/cinemareader.py
cinema_python/adaptors/__init__.py
cinema_python/adaptors/vtk/vtk_explorers.py
cinema_python/adaptors/vtk/__init__.py
cinema_python/adaptors/explorers.py
"

extract_source () {
    git_archive
}

. "${BASH_SOURCE%/*}/../update-common.sh"
