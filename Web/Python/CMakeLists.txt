set(files
  paraview/web/__init__.py
  paraview/web/_default_proxies.py
  paraview/web/camera.py
  paraview/web/data_converter.py
  paraview/web/data_writer.py
  paraview/web/dataset_builder.py
  paraview/web/decorators.py
  paraview/web/helper.py
  paraview/web/protocols.py
  paraview/web/pv_wslink.py
  paraview/web/test_server.py
  paraview/web/vtkjs_helper.py
)

vtk_module_add_python_package(ParaView::PVWebPython
  FILES               ${files}
  PACKAGE             "paraview.web"
  MODULE_DESTINATION  "${PARAVIEW_PYTHON_SITE_PACKAGES_SUFFIX}")

vtk_module_add_python_module(ParaView::PVWebPython
  PACKAGES "paraview.web")
