IF (ParaView_SOURCE_DIR)
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
    
include_directories(
  ${PARAVIEW_GUI_INCLUDE_DIRS}
  ${PARAVIEW_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
)

set(SM_SOURCES
  vtkSMCustomBoundsDomain.cxx
  vtkSMPointSpriteRepresentationProxy.cxx
  vtkSMSpriteTextureProxy.cxx
  vtkPointSpriteRepresentation.cxx
  )

set(SM_XMLS_Rendering
  CSCS_PointSprite_Rendering.xml
  CSCS_PointSpriteRepresentation.xml
  )
  
set(SM_XMLS_Graphics
  CSCS_PointSprite_Graphics.xml
  )

set(PLUGIN_NAME PointSprite_Plugin)

set(CS_WRAP_Rendering)
include(${PointSpritePlugin_BINARY_DIR}/Rendering/vtkPointSprite_RenderingKit.cmake)
foreach(class ${VTK_POINTSPRITE_RENDERING_CLASSES})
  set(CS_WRAP_Rendering ${CS_WRAP_Rendering} ${VTK_POINTSPRITE_RENDERING_HEADER_DIR}/${class}.h)
endforeach(class)

set(CS_WRAP_Graphics)
include(${PointSpritePlugin_BINARY_DIR}/Graphics/vtkPointSprite_GraphicsKit.cmake)
foreach(class ${VTK_POINTSPRITE_GRAPHICS_CLASSES})
  set(CS_WRAP_Graphics ${CS_WRAP_Graphics} ${VTK_POINTSPRITE_GRAPHICS_HEADER_DIR}/${class}.h)
endforeach(class)


IF(PARAVIEW_BUILD_QT_GUI)
  INCLUDE(${QT_USE_FILE}) 

    include_directories(
      ${QT_INCLUDES} 
      ${CMAKE_CURRENT_SOURCE_DIR}/Qvis
    )
    
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
      pqTransferFunctionDialog.h
      pqTransferFunctionEditor.h
      pqDoubleEdit.h
      pqPointSpriteTextureComboBox.h
      )
       
    set(Plugin_SRCS
      pqPointSpriteDisplayPanelDecorator.cxx
      pqTransferFunctionDialog.cxx
      pqTransferFunctionEditor.cxx
      pqDoubleEdit.cxx
      pqPointSpriteTextureComboBox.cxx
      )
    
    
    add_paraview_display_panel_decorator(
      IFACES
      IFACE_SRCS
      CLASS_NAME pqPointSpriteDisplayPanelDecorator
      PANEL_TYPES pqDisplayProxyEditor)
    
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
ELSE(PARAVIEW_BUILD_QT_GUI)
    add_paraview_plugin(${PLUGIN_NAME} "1.0"
        SERVER_MANAGER_SOURCES 
            "${SM_SOURCES};${CS_WRAP_Rendering};${CS_WRAP_Graphics}"
        SERVER_MANAGER_XML 
            "${SM_XMLS_Rendering};${SM_XMLS_Graphics}"
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)

target_link_libraries(${PLUGIN_NAME}
  PointSprite_Rendering
  PointSprite_Graphics
)
