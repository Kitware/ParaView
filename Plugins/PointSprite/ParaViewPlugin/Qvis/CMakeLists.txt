include_directories(${CMAKE_CURRENT_SOURCE_DIR})

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/QvisConfigure.h.in"
  "${PROJECT_BINARY_DIR}/QvisConfigure.h"
  )

include_directories(${PROJECT_BINARY_DIR})

set(Qvis_SRCS
  QvisAbstractOpacityBar.cpp
  QvisGaussianOpacityBar.cpp
  QvisScribbleOpacityBar.cpp
  QvisSpectrumBar.cpp
  QvisColorGridWidget.cpp
  DataNode.cpp
  ColorControlPoint.cpp
  ColorControlPointList.cpp
  AttributeSubject.cpp
  AttributeGroup.cpp
  Subject.cpp
  GaussianControlPoint.cpp
  GaussianControlPointList.cpp
  SimpleObserver.cpp
  VisItException.cpp
  ImproperUseException.cpp
  VolumeAttributes.cpp
  ColorTableAttributes.cpp
 )
 
qt4_wrap_cpp(MOC_SRCS 
  QvisAbstractOpacityBar.h
  QvisGaussianOpacityBar.h
  QvisScribbleOpacityBar.h
  QvisSpectrumBar.h
  QvisColorGridWidget.h
)
 
add_library(Qvis ${Qvis_SRCS} ${MOC_SRCS})
