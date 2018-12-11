paraview_add_plugin(EULATestPlugin
  REQUIRED_ON_CLIENT
  VERSION             "0.1"
  SERVER_MANAGER_XML  EULATestPlugin.xml
  EULA                EULA.txt
  XML_DOCUMENTATION   OFF)
