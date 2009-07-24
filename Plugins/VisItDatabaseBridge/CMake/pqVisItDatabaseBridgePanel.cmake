# +---------------------------------------------------------------------------+
# |                                                                           |
# |                          vtk VisIt Database Bridge                        |
# |                                                                           |
# +---------------------------------------------------------------------------+

# Pipeline Bridge Target 
link_directories(VisItReaderPlugin ${VISIT_LIB_PATH})
include_directories(VisItReaderPlugin ${VISIT_INCLUDE_PATH})

# +---------------------------------------------------------------------------+
# |                                                                           |
# |                   VisIt Database Bridge PQ Panel                          |
# |                                                                           |
# +---------------------------------------------------------------------------+
#
# Builds the custom panel for the database bridge, this file is 
# configured during the boostrap process based on what plugins
# are found.
#

set(PANEL_ASSOCIATION
  ANALYZEVisItDatabaseBridge
  ANSYSVisItDatabaseBridge
  AUXFileVisItDatabaseBridge
  AugDecompVisItDatabaseBridge
  BOVVisItDatabaseBridge
  BOWVisItDatabaseBridge
  Boxlib2DVisItDatabaseBridge
  Boxlib3DVisItDatabaseBridge
  CEAucdVisItDatabaseBridge
  CGNSVisItDatabaseBridge
  CMATVisItDatabaseBridge
  CTRLVisItDatabaseBridge
  CaleVisItDatabaseBridge
  ChomboVisItDatabaseBridge
  ClawVisItDatabaseBridge
  CosmosVisItDatabaseBridge
  CosmosPPVisItDatabaseBridge
  Curve2DVisItDatabaseBridge
  DDCMDVisItDatabaseBridge
  DuneVisItDatabaseBridge
  Dyna3DVisItDatabaseBridge
  EnzoVisItDatabaseBridge
  ExtrudedVolVisItDatabaseBridge
  FITSVisItDatabaseBridge
  FLASHVisItDatabaseBridge
  GGCMVisItDatabaseBridge
  GTCVisItDatabaseBridge
  H5NimrodVisItDatabaseBridge
  H5PartVisItDatabaseBridge
  HexVisItDatabaseBridge
  KullLiteVisItDatabaseBridge
  LinesVisItDatabaseBridge
  M3DVisItDatabaseBridge
  MM5VisItDatabaseBridge
  MirandaVisItDatabaseBridge
  NASTRANVisItDatabaseBridge
  NETCDFVisItDatabaseBridge
  Nek3DVisItDatabaseBridge
  PATRANVisItDatabaseBridge
  PDBVisItDatabaseBridge
  PFLOTRANVisItDatabaseBridge
  PLOT2DVisItDatabaseBridge
  PixieVisItDatabaseBridge
  PlainTextVisItDatabaseBridge
  Point3DVisItDatabaseBridge
  RectVisItDatabaseBridge
  S3DVisItDatabaseBridge
  SAMIVisItDatabaseBridge
  SAMRAIVisItDatabaseBridge
  SARVisItDatabaseBridge
  SASVisItDatabaseBridge
  ShapefileVisItDatabaseBridge
  SiloVisItDatabaseBridge
  SpheralVisItDatabaseBridge
  TFTVisItDatabaseBridge
  TSurfVisItDatabaseBridge
  TecplotVisItDatabaseBridge
  TetradVisItDatabaseBridge
  UNICVisItDatabaseBridge
  VASPVisItDatabaseBridge
  VLIVisItDatabaseBridge
  Vis5DVisItDatabaseBridge
  VistaVisItDatabaseBridge
  XmdvVisItDatabaseBridge
  ZeusMPVisItDatabaseBridge
  ZipWrapperVisItDatabaseBridge
)

QT4_WRAP_CPP(MOC_SRCS pqVisItDatabaseBridgePanel.h)
QT4_WRAP_UI(UI_SRCS pqVisItDatabaseBridgeForm.ui)
ADD_PARAVIEW_OBJECT_PANEL(
  IFACES IFACE_SRCS
  CLASS_NAME pqVisItDatabaseBridgePanel
  XML_NAME ${PANEL_ASSOCIATION}
  XML_GROUP sources)

ADD_PARAVIEW_PLUGIN(
  VisItReaderPlugin "1.0"
  GUI_INTERFACES ${IFACES}
  SERVER_MANAGER_SOURCES vtkVisItDatabaseBridge.cxx 
  SERVER_MANAGER_XML vtkVisItDatabaseBridgeServerManager.xml
  GUI_RESOURCE_FILES vtkVisItDatabaseBridgeClient.xml
  SOURCES pqVisItDatabaseBridgePanel.cxx ${MOC_SRCS} ${UI_SRCS} ${IFACE_SRCS})

target_link_libraries(VisItReaderPlugin ${VISIT_LIBS} vtkCommon vtkFiltering vtkGraphics vtkParallel vtkVisItDatabase)

# Platform
if (UNIX OR CYGWIN)
  message(STATUS "Configuring vtkVisItDatabase for use on Linux.")
  set_source_files_properties(VisItReaderPlugin COMPILE_FLAGS ${COMPILE_FLAGS} "-DUNIX")
else (UNIX OR CYGWIN)
  message(STATUS "Configuring  vtkVisItDatabase for use on Windows.")
endif (UNIX OR CYGWIN)
# MPI
if (VISIT_WITH_MPI)
  message(STATUS "Configure vtkVisItDatabase for VisIt built with MPI.")
  set_source_files_properties(VisItReaderPlugin COMPILE_FLAGS ${COMPILE_FLAGS} "-DMPI")
endif (VISIT_WITH_MPI)

install (TARGETS VisItReaderPlugin
  DESTINATION "${PV_INSTALL_BIN_DIR}/plugins/VisItReaderPlugin"
  COMPONENT Runtime)

install (TARGETS VisItReaderPlugin
  DESTINATION "${PV_INSTALL_BIN_DIR}/plugins/VisItReaderPlugin"
  COMPONENT Runtime)

