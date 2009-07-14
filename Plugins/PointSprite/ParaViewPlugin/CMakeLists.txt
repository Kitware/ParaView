IF (ParaView_SOURCE_DIR)
  INCLUDE(${QT_USE_FILE}) 
  INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${PARAVIEW_GUI_INCLUDE_DIRS}
    ${PARAVIEW_KWSYS_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
    ${MPI_INCLUDE_PATH}
  )
ELSE (ParaView_SOURCE_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF (ParaView_SOURCE_DIR)


set(QVIS_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisAbstractOpacityBar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisGaussianOpacityBar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisScribbleOpacityBar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisSpectrumBar.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisColorGridWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/DataNode.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/ColorControlPoint.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/ColorControlPointList.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/AttributeSubject.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/AttributeGroup.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/Subject.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/GaussianControlPoint.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/GaussianControlPointList.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/SimpleObserver.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/VisItException.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/ImproperUseException.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/VolumeAttributes.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/ColorTableAttributes.cpp
 )
 
qt4_wrap_cpp(QVIS_MOC_SRCS 
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisAbstractOpacityBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisGaussianOpacityBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisScribbleOpacityBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisSpectrumBar.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Qvis/QvisColorGridWidget.h
)
  
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Qvis)

set(IFACES
  pqPointSpriteDisplayPanelDecorator
)

set(UI_FORMS
  Resources/pqPointSpriteDisplayPanelDecorator.ui
  Resources/pqTransferFunctionEditor.ui
  Resources/pqTransferFunctionDialog.ui
)
    
qt4_wrap_ui(UI_SRCS 
  ${UI_FORMS}
)
   
qt4_wrap_cpp(MOC_SRCS 
  pqPointSpriteDisplayPanelDecorator.h
  pqPointSpriteDisplayPanelDecoratorImplementation.h
  pqDisplayArrayWidget.h
  pqTransferFunctionDialog.h
  pqTransferFunctionEditor.h
  pqDoubleEdit.h
  pqPointSpriteTextureComboBox.h
  )
   
set(Plugin_SRCS
  pqPointSpriteDisplayPanelDecoratorImplementation.cxx
  pqPointSpriteDisplayPanelDecorator.cxx
  pqDisplayArrayWidget.cxx
  pqTransferFunctionDialog.cxx
  pqTransferFunctionEditor.cxx
  pqDoubleEdit.cxx
  pqPointSpriteTextureComboBox.cxx
  )

set(SM_SOURCES
  vtkSMPointSpriteRepresentationProxy.cxx
  vtkSMSpriteTextureProxy.cxx
  )

set(SM_XMLS_Rendering
  CSCS_PointSprite_Rendering.xml
  CSCS_PointSpriteRepresentation.xml
  )
  
set(SM_XMLS_Graphics
  CSCS_PointSprite_Graphics.xml
  )

set(PLUGIN_NAME CSCS_PointSprite_GUIPlugin)

set(CS_WRAP_Rendering)
include(${PointSpritePlugin_BINARY_DIR}/Rendering/vtkCSCS_PointSprite_RenderingKit.cmake)
foreach(class ${VTK_POINTSPRITE_RENDERING_CLASSES})
  set(CS_WRAP_Rendering ${CS_WRAP_Rendering} ${VTK_POINTSPRITE_RENDERING_HEADER_DIR}/${class}.h)
endforeach(class)

set(CS_WRAP_Graphics)
include(${PointSpritePlugin_BINARY_DIR}/Graphics/vtkCSCS_PointSprite_GraphicsKit.cmake)
foreach(class ${VTK_POINTSPRITE_GRAPHICS_CLASSES})
  set(CS_WRAP_Graphics ${CS_WRAP_Graphics} ${VTK_POINTSPRITE_GRAPHICS_HEADER_DIR}/${class}.h)
endforeach(class)

include_directories(${QT_INCLUDES} ${PARAVIEW_GUI_INCLUDE_DIRS}
  ${PARAVIEW_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

add_paraview_plugin(${PLUGIN_NAME} "1.0"
  SERVER_MANAGER_SOURCES 
    "${SM_SOURCES};${CS_WRAP_Rendering};${CS_WRAP_Graphics}"
  SERVER_MANAGER_XML 
    "${SM_XMLS_Rendering};${SM_XMLS_Graphics}"
  GUI_INTERFACES 
    ${IFACES} 
  GUI_SOURCES 
    ${Plugin_SRCS} 
    ${QVIS_SRCS} 
    ${QVIS_MOC_SRCS} 
    ${MOC_SRCS} 
    ${UI_SRCS} 
    ${IFACE_SRCS}
)

add_paraview_plugin(${PLUGIN_NAME}_Server "1.0"
  SERVER_MANAGER_SOURCES 
    ${CS_WRAP_Graphics}
  SERVER_MANAGER_XML 
    ${SM_XMLS_Graphics}
)

target_link_libraries(${PLUGIN_NAME}
  CSCS_PointSprite_Rendering
  CSCS_PointSprite_Graphics
)

target_link_libraries(${PLUGIN_NAME}_Server
  CSCS_PointSprite_Graphics
)
    
