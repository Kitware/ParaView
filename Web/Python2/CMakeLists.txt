set(files
  paraview/web/ipython.py
  paraview/web/pv_web_catalyst.py
  paraview/web/webgl.py)

vtk_module_add_python_package(ParaView::PVWebPython2
  FILES               ${files}
  PACKAGE             "paraview.web"
  MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(ParaView::PVWebPython2
  PACKAGES "paraview.web")
