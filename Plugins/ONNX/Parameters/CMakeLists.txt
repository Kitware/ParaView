vtk_module_find_package(
  PACKAGE     "Qt${PARAVIEW_QT_MAJOR_VERSION}"
  COMPONENTS  Widgets)

set(CMAKE_AUTOMOC 1)

vtk_module_add_module(ONNXPlugin::ONNXParameters
  NOWRAP_HEADERS
    pqNumericParameter.h
  NOWRAP_CLASSES
    pqONNXJsonVerify
  CLASSES
    pqONNXInputParameter
    pqONNXOutputParameters
    pqONNXTimeParameter
  )

vtk_module_definitions(ONNXPlugin::ONNXParameters PRIVATE QT_NO_KEYWORDS)
