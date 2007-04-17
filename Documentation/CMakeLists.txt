set(PARAVIEW_HTML_DOCUMENTATION
  CellData.png
  ColorEditor.png
  ColorLegend.png
  ColorLegendToolbarButton.png
  ColorOpacityEditor.png
  ColorScalePresets.png
  Display.html
  DisplayColorByArray.png
  DisplayColorSolid.png
  DisplayStyle.png
  DisplayTransformation.png
  DisplayView.png
  index.html
  Information.html
  InformationTab.png
  ObjectInspector.html
  ParaViewDoc.css
  ParaViewLogo.png
  PointData.png
  Properties.html
  PropertiesButtons.png
)

foreach(filename ${PARAVIEW_HTML_DOCUMENTATION})
  configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${filename}"
    "${CMAKE_CURRENT_BINARY_DIR}/${filename}"
    COPYONLY)
endforeach(filename)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/pqClient.adp"
  "${ParaView_BINARY_DIR}/pqClient.adp"
  COPYONLY)

set(dir "${ParaView_BINARY_DIR}/Documentation/")
install(DIRECTORY ${ParaView_BINARY_DIR}/Documentation/ 
        DESTINATION ${PV_EXE_INSTALL_CM24}/Documentation 
        COMPONENT Runtime
        REGEX ".html")

install(FILES ${ParaView_BINARY_DIR}/pqClient.adp 
  DESTINATION ${PV_EXE_INSTALL_CM24} COMPONENT Runtime )
