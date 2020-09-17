set(files
  paraview/tpl/cinema_python/adaptors/explorers.py
  paraview/tpl/cinema_python/adaptors/__init__.py
  paraview/tpl/cinema_python/adaptors/paraview/cinemareader.py
  paraview/tpl/cinema_python/adaptors/paraview/__init__.py
  paraview/tpl/cinema_python/adaptors/paraview/progress.py
  paraview/tpl/cinema_python/adaptors/paraview/pv_explorers.py
  paraview/tpl/cinema_python/adaptors/paraview/pv_introspect.py
  paraview/tpl/cinema_python/database/file_store.py
  paraview/tpl/cinema_python/database/__init__.py
  paraview/tpl/cinema_python/database/oexr_helper.py
  paraview/tpl/cinema_python/database/raster_wrangler.py
  paraview/tpl/cinema_python/database/store.py
  paraview/tpl/cinema_python/database/vti_store.py
  paraview/tpl/cinema_python/images/camera_utils.py
  paraview/tpl/cinema_python/images/compositor.py
  paraview/tpl/cinema_python/images/__init__.py
  paraview/tpl/cinema_python/images/layer_rasters.py
  paraview/tpl/cinema_python/images/lookup_tables.py
  paraview/tpl/cinema_python/images/querymaker.py
  paraview/tpl/cinema_python/images/querymaker_specb.py
  paraview/tpl/cinema_python/__init__.py)

vtk_module_add_python_package(ParaView::CinemaPython
  FILES               ${files}
  PACKAGE             "paraview.tpl.cinema_python"
  MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(ParaView::CinemaPython
  PACKAGES
    "paraview.tpl.cinema_python")
