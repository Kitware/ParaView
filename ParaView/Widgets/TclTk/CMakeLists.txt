SET (files
  lib/tcl8.4/tclIndex
  lib/tcl8.4/auto.tcl
  lib/tcl8.4/init.tcl
  lib/tk8.4/tclIndex
  lib/tk8.4/button.tcl
  lib/tk8.4/comdlg.tcl
  lib/tk8.4/entry.tcl
  lib/tk8.4/listbox.tcl
  lib/tk8.4/menu.tcl
  lib/tk8.4/optMenu.tcl
  lib/tk8.4/panedwindow.tcl
  lib/tk8.4/scale.tcl
  lib/tk8.4/scrlbar.tcl
  lib/tk8.4/spinbox.tcl
  lib/tk8.4/tearoff.tcl
  lib/tk8.4/text.tcl
  lib/tk8.4/tk.tcl
  lib/tk8.4/tkfbox.tcl
)

IF(CMAKE_CONFIGURATION_TYPES)
  SET(WIDGETS_CONFIG_TYPES ${CMAKE_CONFIGURATION_TYPES})
ELSE(CMAKE_CONFIGURATION_TYPES)
  SET(WIDGETS_CONFIG_TYPES .)
ENDIF(CMAKE_CONFIGURATION_TYPES)
  
IF (CXX_TEST_PATH)  
  FOREACH(config ${WIDGETS_CONFIG_TYPES})
    FOREACH (file ${files})
      CONFIGURE_FILE(
        ${KWWIDGETS_SOURCE_DIR}/TclTk/${file}
        ${CXX_TEST_PATH}/${config}/TclTk/${file}
        COPYONLY
      )
    ENDFOREACH (file)
  ENDFOREACH(config)
ENDIF (CXX_TEST_PATH)
