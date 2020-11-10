set(files
  paraview/tpl/cinemasci/cdb/__init__.py
  paraview/tpl/cinemasci/cis/channel.py
  paraview/tpl/cinemasci/cis/colormap.py
  paraview/tpl/cinemasci/cis/image.py
  paraview/tpl/cinemasci/cis/__init__.py
  paraview/tpl/cinemasci/cis/layer.py
  paraview/tpl/cinemasci/cis/read/file.py
  paraview/tpl/cinemasci/cis/read/hdf5.py
  paraview/tpl/cinemasci/cis/read/__init__.py
  paraview/tpl/cinemasci/cis/render.py
  paraview/tpl/cinemasci/cis/write/file.py
  paraview/tpl/cinemasci/cis/write/hdf5.py
  paraview/tpl/cinemasci/cis/write/__init__.py
  paraview/tpl/cinemasci/__init__.py
  paraview/tpl/cinemasci/pynb/__init__.py
  paraview/tpl/cinemasci/server/cinema_test.html
  paraview/tpl/cinemasci/server/error_no-database.html
  paraview/tpl/cinemasci/server/__init__.py
  paraview/tpl/cinemasci/server/__main__.py
  paraview/tpl/cinemasci/version.py
  paraview/tpl/cinemasci/viewers/cinema/explorer/2.0/css/viewer.css
  paraview/tpl/cinemasci/viewers/cinema/explorer/2.0/databases.json
  paraview/tpl/cinemasci/viewers/cinema/explorer/2.0/js/main.js
  paraview/tpl/cinemasci/viewers/cinema_explorer.html
  paraview/tpl/cinemasci/viewers/cinema/lib/d3.v4.min.js
  paraview/tpl/cinemasci/viewers/cinema/lib/databases.js
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/copyright.md
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/css/common.css
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/css/range-css.css
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/css/range-css.textClipping
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/css/select-css.css
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/css/view.css
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/databases.json
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/images/empty.png
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/license.md
  paraview/tpl/cinemasci/viewers/cinema/view/2.0/version.md
  paraview/tpl/cinemasci/viewers/cinema_view.html
  paraview/tpl/cinemasci/viewers/citation.md
  paraview/tpl/cinemasci/viewers/__init__.py
  paraview/tpl/cinemasci/viewers/license.md
  paraview/tpl/cinemasci/viewers/readme.md
  paraview/tpl/cinemasci/viewers/test_explorer.html
  paraview/tpl/cinemasci/viewers/test_view.html
  paraview/tpl/cinemasci/viewers/version.md)

vtk_module_add_python_package(ParaView::CinemaSci
  FILES               ${files}
  PACKAGE             "paraview.tpl.cinemasci"
  MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(ParaView::CinemaSci
  PACKAGES
    "paraview.tpl.cinemasci")

set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "pandas>=0.21")
