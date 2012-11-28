include_directories(${VTK_INCLUDE_DIRS}
  ${PointSpritePlugin_SOURCE_DIR}/Graphics
  ${PointSpritePlugin_BINARY_DIR}/Graphics
  ${PointSpritePlugin_SOURCE_DIR}/Rendering
  ${PointSpritePlugin_BINARY_DIR}/Rendering
  ${PointSpritePlugin_SOURCE_DIR}/Qvis
  ${PointSpritePlugin_BINARY_DIR}/Qvis)
    
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

IF(PARAVIEW_BUILD_QT_GUI)
    include_directories(${QT_INCLUDES})
    
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
        ${SM_SOURCES}
      SERVER_MANAGER_XML 
        "${SM_XMLS_Rendering};${SM_XMLS_Graphics}"
      GUI_INTERFACES 
        ${IFACES} 
      GUI_SOURCES 
        ${Plugin_SRCS} 
        ${MOC_SRCS} 
        ${UI_SRCS} 
        ${IFACE_SRCS}
    )
    target_link_libraries(${PLUGIN_NAME} vtkQvis)

ELSE(PARAVIEW_BUILD_QT_GUI)
    add_paraview_plugin(${PLUGIN_NAME} "1.0"
        SERVER_MANAGER_SOURCES 
            ${SM_SOURCES}
        SERVER_MANAGER_XML 
            "${SM_XMLS_Rendering};${SM_XMLS_Graphics}"
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)

target_link_libraries(${PLUGIN_NAME}
  vtkPointSpriteGraphics
  vtkPointSpriteRendering
)
