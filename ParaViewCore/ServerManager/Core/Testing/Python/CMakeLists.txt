# Ad python script names here
SET( PY_TESTS_NO_BASELINE
  Settings
)

IF (PARAVIEW_DATA_ROOT)
  FOREACH (tfile ${PY_TESTS_NO_BASELINE})
    ADD_TEST(NAME "${tfile}-ServerManagerPython"
      COMMAND $<TARGET_FILE:pvpython>
        --enable-bt
        ${CMAKE_CURRENT_SOURCE_DIR}/${tfile}.py
        -D ${PARAVIEW_DATA_ROOT}
        -T ${ParaView_BINARY_DIR}/Testing/Temporary
        -S ${SMSTATE_FILE_DIR}
        )
    SET_TESTS_PROPERTIES(${tfile}-ServerManagerPython PROPERTIES LABELS "PARAVIEW")
  ENDFOREACH(tfile)
ENDIF ()
