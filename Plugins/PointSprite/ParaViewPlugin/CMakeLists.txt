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
      Resources/pqPointSpriteControls.ui
      Resources/pqPointSpriteDisplayPanelDecorator.ui
      Resources/pqTransferFunctionDialog.ui
      Resources/pqTransferFunctionEditor.ui
    )
        
    qt4_wrap_ui(UI_SRCS 
      ${UI_FORMS}
    )
       
    qt4_wrap_cpp(MOC_SRCS 
      pqDoubleEdit.h
      pqPointSpriteControls.h
      pqPointSpriteDisplayPanelDecorator.h
      pqPointSpriteTextureComboBox.h
      pqTransferFunctionDialog.h
      pqTransferFunctionEditor.h
      )
       
    set(Plugin_SRCS
      pqDoubleEdit.cxx
      pqPointSpriteControls.cxx
      pqPointSpriteDisplayPanelDecorator.cxx
      pqPointSpriteTextureComboBox.cxx
      pqTransferFunctionDialog.cxx
      pqTransferFunctionEditor.cxx
      )

    # for legacy panels (in Object Inspector). 
    add_paraview_display_panel_decorator(
      IFACES
      IFACE_SRCS
      CLASS_NAME pqPointSpriteDisplayPanelDecorator
      PANEL_TYPES pqDisplayProxyEditor)


    # for the new Properties Panel
    add_paraview_property_group_widget(
      ifaces_2
      iface_sources_2
      TYPE "point_sprite_controls"
      CLASS_NAME pqPointSpriteControls)

    add_paraview_plugin(${PLUGIN_NAME} "1.0"
      SERVER_MANAGER_SOURCES 
        ${SM_SOURCES}
      SERVER_MANAGER_XML 
        ${SM_XMLS_Rendering}
        ${SM_XMLS_Graphics}
      GUI_INTERFACES 
        ${IFACES} 
        ${ifaces_2}
      GUI_SOURCES 
        ${Plugin_SRCS} 
        ${MOC_SRCS} 
        ${UI_SRCS} 
        ${IFACE_SRCS}
        ${iface_sources_2})
    target_link_libraries(${PLUGIN_NAME} vtkQvis)

ELSE(PARAVIEW_BUILD_QT_GUI)
    add_paraview_plugin(${PLUGIN_NAME} "1.0"
        SERVER_MANAGER_SOURCES 
            ${SM_SOURCES}
        SERVER_MANAGER_XML 
            ${SM_XMLS_Rendering}
            ${SM_XMLS_Graphics}
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)
