SET (files
  lib/tcl8.4/tclIndex
  lib/tcl8.4/auto.tcl
  lib/tcl8.4/init.tcl
  lib/tk8.4/tclIndex
  lib/tk8.4/button.tcl
  lib/tk8.4/entry.tcl
  lib/tk8.4/listbox.tcl
  lib/tk8.4/menu.tcl
  lib/tk8.4/panedwindow.tcl
  lib/tk8.4/scale.tcl
  lib/tk8.4/scrlbar.tcl
  lib/tk8.4/spinbox.tcl
  lib/tk8.4/tearoff.tcl
  lib/tk8.4/text.tcl
  lib/tk8.4/tk.tcl
)

IF (CXX_TEST_PATH)
  FOREACH (file ${files})
    CONFIGURE_FILE(
      ${KWWIDGETS_SOURCE_DIR}/TclTk/${file}
      ${CXX_TEST_PATH}/${CMAKE_CFG_INTDIR}/TclTk/${file}
      COPYONLY
    )
  ENDFOREACH (file ${files})
ENDIF (CXX_TEST_PATH)
