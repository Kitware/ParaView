set(PARAVIEW_HTML_DOCUMENTATION
  3DView.png
  AfterSwapViews.png
  Animation.html
  AnimationInspector.png
  AnimationToolbar.png
  BeforeSwapViews.png
  CellData.png
  ChangeInput.png
  CloseViewButton.png
  ColorEditor.png
  ColorLegend.png
  ColorLegendToolbarButton.png
  ColorOpacityEditor.png
  ColorScalePresets.png
  CreateViewButtons.png
  Display.html
  DisplayColorByArray.png
  DisplayColorSolid.png
  DisplayStyle.png
  DisplayTransformation.png
  DisplayView.png
  Exponential.png
  HistogramDisplayTab.png
  HistogramView.png
  index.html
  Information.html
  InformationTab.png
  LookmarkButton.png
  MaximizeViewButton.png
  ObjectInspector.html
  ParaViewDoc.css
  ParaViewLogo.png
  PipelineBrowser.html
  PipelineBrowser.png
  PointData.png
  Properties.html
  PropertiesButtons.png
  Ramp.png
  RestoreViewButton.png
  RightClickFilter.png
  Sinusoidal.png
  SplitViewHorizontalButton.png
  SplitViewVerticalButton.png
  Step.png
  VCRBack.png
  VCRFirst.png
  VCRForward.png
  VCRLast.png
  VCRLoop.png
  VCRPlay.png
  VCRPause.png
  Views.html
  ViewTitleBarButtons.png
  XYPlotDisplayTab.png
  XYPlotView.png
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
