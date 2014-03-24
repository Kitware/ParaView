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

  set(UI_FORMS
    Resources/pqPointSpriteControls.ui
    Resources/pqTransferFunctionDialog.ui
    Resources/pqTransferFunctionEditor.ui
  )

  set(PLUGIN_MOC_SRCS
      pqDisplayArrayWidget.h
      pqDoubleEdit.h
      pqPointSpriteControls.h
      pqPointSpriteTextureComboBox.h
      pqTransferFunctionDialog.h
      pqTransferFunctionEditor.h
      )
  if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    include(ParaViewQt5)
    include_directories(${Qt5Core_INCLUDE_DIRS})
    qt5_wrap_ui(UI_SRCS
      ${UI_FORMS}
    )

    qt5_wrap_cpp(MOC_SRCS
      ${PLUGIN_MOC_SRCS})
  else ()
    include(${QT_USE_FILE})
    include_directories(${QT_INCLUDES})
    qt4_wrap_ui(UI_SRCS
      ${UI_FORMS}
    )

    qt4_wrap_cpp(MOC_SRCS
      ${PLUGIN_MOC_SRCS})
  endif ()

    set(Plugin_SRCS
      pqDisplayArrayWidget.cxx
      pqDoubleEdit.cxx
      pqPointSpriteControls.cxx
      pqPointSpriteTextureComboBox.cxx
      pqTransferFunctionDialog.cxx
      pqTransferFunctionEditor.cxx
      )

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
    target_link_libraries(${PLUGIN_NAME} LINK_PUBLIC vtkQvis)

ELSE(PARAVIEW_BUILD_QT_GUI)
    add_paraview_plugin(${PLUGIN_NAME} "1.0"
        SERVER_MANAGER_SOURCES
            ${SM_SOURCES}
        SERVER_MANAGER_XML
            ${SM_XMLS_Rendering}
            ${SM_XMLS_Graphics}
    )
ENDIF(PARAVIEW_BUILD_QT_GUI)
target_link_libraries(${PLUGIN_NAME} LINK_PUBLIC vtkPointSpriteGraphics vtkPointSpriteRendering)
