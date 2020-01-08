vtk_module_test_data(
  Data/,REGEX:poisson_3d.*\\.h5
  Data/SPCTH/,REGEX:.*
  Data/iron_protein.vtk
  Data/sonic.pht
  Data/sonicgeom.dat.1
  Data/sonicrestart.1)

if (PARAVIEW_USE_PYTHON)
  vtk_module_test_data(
    Data/FileSeries/,REGEX:.*
    Data/PythonProgrammableFilterParameters.xml
    Data/TestRepresentationTypePlugin.xml
    Data/blow.vtk
    Data/blow_data.myvtk
    Data/bot2.wrl
    Data/can.ex2
    Data/disk_out_ref.ex2
    Data/dualSphereAnimation/,REGEX:.*
    Data/dualSphereAnimation4.pvd
    Data/elements.vtu
    Data/multicomb_0.vts
    Data/poisson_3dall_2.xmf
    Data/quadraticTetra01.vtu)

  ExternalData_Expand_Arguments(ParaViewData _
    # Baselines
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:SaveScreenshotBarPlot.*\\.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:SaveScreenshotOutline.*\\.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:SaveScreenshotLinePlot.*\\.png}"
    "DATA{${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline/,REGEX:SaveScreenshotSurface.*\\.png}"
    )
  add_subdirectory(Python)
endif ()
