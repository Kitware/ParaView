if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  set (Qt5_FIND_COMPONENTS Widgets)
  include (ParaViewQt5)
  include_directories(${Qt5Core_INCLUDE_DIRS})
else ()
  include (${QT_USE_FILE})
  include_directories(${QT_INCLUDES})
endif ()

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

if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
  qt5_wrap_cpp(MOC_SRCS
    ${Qvis_MOC_SRCS})
else ()
  qt4_wrap_cpp(MOC_SRCS
    ${Qvis_MOC_SRCS})
endif ()

set(vtkQvis_NO_HeaderTest 1)

vtk_module_library(vtkQvis ${Qvis_SRCS} ${MOC_SRCS})
target_link_libraries(vtkQvis LINK_PRIVATE ${QT_LIBRARIES})
