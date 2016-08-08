include(ParaViewQt)
pv_find_package_qt(qt_targets REQUIRED QT4_COMPONENTS QtGui QT5_COMPONENTS Widgets)
set(Qvis_SRCS
  QvisAbstractOpacityBar.cpp
  QvisGaussianOpacityBar.cpp
  QvisScribbleOpacityBar.cpp
  QvisSpectrumBar.cpp
  QvisColorGridWidget.cpp

  AttributeGroup.cpp
  AttributeSubject.cpp
  ColorControlPoint.cpp
  ColorControlPointList.cpp
  ColorTableAttributes.cpp
  DataNode.cpp
  GaussianControlPoint.cpp
  GaussianControlPointList.cpp
  ImproperUseException.cpp
  SimpleObserver.cpp
  Subject.cpp
  VisItException.cpp
  VolumeAttributes.cpp
 )

set(Qvis_MOC_SRCS
  QvisAbstractOpacityBar.h
  QvisGaussianOpacityBar.h
  QvisScribbleOpacityBar.h
  QvisSpectrumBar.h
  QvisColorGridWidget.h
)

pv_qt_wrap_cpp(MOC_SRCS ${Qvis_MOC_SRCS})
set(vtkQvis_NO_HeaderTest 1)
vtk_module_library(vtkQvis ${Qvis_SRCS} ${MOC_SRCS})
target_link_libraries(vtkQvis LINK_PRIVATE ${qt_targets})
