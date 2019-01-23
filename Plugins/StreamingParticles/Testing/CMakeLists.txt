# TODO: Update

if (FALSE AND PARAVIEW_BUILD_QT_GUI AND PARAVIEW_ENABLE_COSMOTOOLS)
  ExternalData_Expand_Arguments("ParaViewData" _
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/adaptive-cosmo/,REGEX:.*}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/multiresolution-streaming/,REGEX:.*}")

  paraview_add_client_tests(
    COMMAND --client ${CLIENT_EXECUTABLE}
            --enable-bt
            -dr
            --enable-streaming
            --test-directory=${PARAVIEW_TEST_DIR}
            --test-plugin=StreamingParticles
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS StreamingParticles.xml
    )
  paraview_test_load_data_dirs(NewGenericIOStreaming
    multiresolution-streaming
    )
  paraview_add_client_tests(
    COMMAND --client ${CLIENT_EXECUTABLE}
            --enable-bt
            -dr
            --mpi
            --enable-streaming
            --test-directory=${PARAVIEW_TEST_DIR}
            --test-plugin=StreamingParticles
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS NewGenericIOStreaming.xml
    )
  paraview_add_client_server_tests(
    COMMAND --server-np 2
            --server $<TARGET_FILE:pvserver>
            --enable-bt --enable-streaming
            --test-plugin=StreamingParticles
            --client ${CLIENT_EXECUTABLE}
            --enable-bt
            -dr
            --mpi
            --enable-streaming
            --test-directory=${PARAVIEW_TEST_DIR}
            --test-plugin=StreamingParticles
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS NewGenericIOStreaming.xml
    )
  paraview_add_client_server_render_tests(
    COMMAND --server-np 2
            --server $<TARGET_FILE:pvserver>
            --enable-bt --enable-streaming
            --test-plugin=StreamingParticles
            --client ${CLIENT_EXECUTABLE}
            --enable-bt
            -dr
            --mpi
            --enable-streaming
            --test-directory=${PARAVIEW_TEST_DIR}
            --test-plugin=StreamingParticles
    BASELINE_DIR ${PARAVIEW_TEST_BASELINE_DIR}
    TEST_SCRIPTS NewGenericIOStreaming.xml
    )
endif ()
