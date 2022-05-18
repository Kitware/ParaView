set(files
  paraview/tpl/cinemasci/__init__.py

  paraview/tpl/cinemasci/cdb/__init__.py

  paraview/tpl/cinemasci/cis/convert/__init__.py
  paraview/tpl/cinemasci/cis/convert/ascent.py

  paraview/tpl/cinemasci/cis/__init__.py
  paraview/tpl/cinemasci/cis/channel.py
  paraview/tpl/cinemasci/cis/cisview.py
  paraview/tpl/cinemasci/cis/colormap.py
  paraview/tpl/cinemasci/cis/imageview.py
  paraview/tpl/cinemasci/cis/layer.py
  paraview/tpl/cinemasci/cis/renderer.py

  paraview/tpl/cinemasci/pynb/cis/__init__.py

  paraview/tpl/cinemasci/server/__init__.py
  paraview/tpl/cinemasci/server/__main__.py
  paraview/tpl/cinemasci/server/cinema_test.html
  paraview/tpl/cinemasci/server/error_no-database.html

  paraview/tpl/cinemasci/viewers/cinema/explorer/2.2/css/viewer.css
  paraview/tpl/cinemasci/viewers/cinema/explorer/2.2/js/main.js
  paraview/tpl/cinemasci/viewers/cinema/explorer/2.2/databases.json

  paraview/tpl/cinemasci/viewers/cinema/lib/2.2/common.css
  paraview/tpl/cinemasci/viewers/cinema/lib/2.2/databases.js
  paraview/tpl/cinemasci/viewers/cinema/lib/cinemacomponents.license.md
  paraview/tpl/cinemasci/viewers/cinema/lib/CinemaComponents.v2.7.1.min.css
  paraview/tpl/cinemasci/viewers/cinema/lib/CinemaComponents.v2.7.1.min.js
  paraview/tpl/cinemasci/viewers/cinema/lib/d3.license.md
  paraview/tpl/cinemasci/viewers/cinema/lib/d3.v4.min.js
  paraview/tpl/cinemasci/viewers/cinema/lib/d3.v5.min.js
  paraview/tpl/cinemasci/viewers/cinema/lib/2.2/img/empty.png
  paraview/tpl/cinemasci/viewers/cinema/lib/2.2/img/cinema_logo_filmreel_named_100px_white.png

  paraview/tpl/cinemasci/viewers/cinema/simple/2.2/css/compare.css

  paraview/tpl/cinemasci/viewers/cinema/view/2.2/css/range-css.css
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/css/range-css.textClipping
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/css/select-css.css
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/cinema_attributes_example.json
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/copyright.md
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/databases.json
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/license.md
  paraview/tpl/cinemasci/viewers/cinema/view/2.2/main.js

  paraview/tpl/cinemasci/viewers/__init__.py
  paraview/tpl/cinemasci/viewers/cinema_explorer.html
  paraview/tpl/cinemasci/viewers/cinema_simple.html
  paraview/tpl/cinemasci/viewers/cinema_view.html
  paraview/tpl/cinemasci/viewers/license.md
  paraview/tpl/cinemasci/viewers/readme.md
  paraview/tpl/cinemasci/viewers/testplan_explorer.html
  paraview/tpl/cinemasci/viewers/testplan_view.html
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
