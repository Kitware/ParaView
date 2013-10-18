find_package(Qt4 REQUIRED QUIET)
include(${QT_USE_FILE})

include_directories(${QT_INCLUDES})

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

qt4_wrap_cpp(MOC_SRCS
  QvisAbstractOpacityBar.h
  QvisGaussianOpacityBar.h
  QvisScribbleOpacityBar.h
  QvisSpectrumBar.h
  QvisColorGridWidget.h
)

set(vtkQvis_NO_HeaderTest 1)

vtk_module_library(vtkQvis ${Qvis_SRCS} ${MOC_SRCS})
target_link_libraries(vtkQvis LINK_PRIVATE ${QT_LIBRARIES})
