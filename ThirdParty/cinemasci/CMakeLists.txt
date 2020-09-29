set(files
  paraview/tpl/cinemasci/cis/colormap.py
  paraview/tpl/cinemasci/cis/channel.py
  paraview/tpl/cinemasci/cis/write/file.py
  paraview/tpl/cinemasci/cis/write/hdf5.py
  paraview/tpl/cinemasci/cis/write/__init__.py
  paraview/tpl/cinemasci/cis/image.py
  paraview/tpl/cinemasci/cis/render.py
  paraview/tpl/cinemasci/cis/read/file.py
  paraview/tpl/cinemasci/cis/read/hdf5.py
  paraview/tpl/cinemasci/cis/read/__init__.py
  paraview/tpl/cinemasci/cis/__init__.py
  paraview/tpl/cinemasci/cis/layer.py
  paraview/tpl/cinemasci/cview/cinema_view.html
  paraview/tpl/cinemasci/cview/cinema_explorer.html
  paraview/tpl/cinemasci/cview/__main__.py
  paraview/tpl/cinemasci/cview/__init__.py
  paraview/tpl/cinemasci/cview/cinema/databases.js
  paraview/tpl/cinemasci/cview/cinema/lib/d3.v4.min.js
  paraview/tpl/cinemasci/cview/cinema/explorer/1.11/css/viewer.css
  paraview/tpl/cinemasci/cview/cinema/explorer/1.11/databases.json
  paraview/tpl/cinemasci/cview/cinema/explorer/1.11/js/main.js
  paraview/tpl/cinemasci/cview/cinema/view/1.2/css/common.css
  paraview/tpl/cinemasci/cview/cinema/view/1.2/css/select-css.css
  paraview/tpl/cinemasci/cview/cinema/view/1.2/css/range-css.textClipping
  paraview/tpl/cinemasci/cview/cinema/view/1.2/css/view.css
  paraview/tpl/cinemasci/cview/cinema/view/1.2/css/range-css.css
  paraview/tpl/cinemasci/cview/cinema/view/1.2/version.md
  paraview/tpl/cinemasci/cdb/__init__.py
  paraview/tpl/cinemasci/cview/cinema/view/1.2/copyright.md
  paraview/tpl/cinemasci/cview/cinema/view/1.2/databases.json
  paraview/tpl/cinemasci/cview/cinema/view/1.2/images/empty.png
  paraview/tpl/cinemasci/cview/cinema/view/1.2/license.md
  paraview/tpl/cinemasci/__init__.py
  paraview/tpl/cinemasci/pynb/__init__.py)

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
