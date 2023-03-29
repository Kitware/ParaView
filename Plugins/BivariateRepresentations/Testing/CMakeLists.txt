ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/Baseline/BivariateNoise_A.png}"
  "DATA{Data/Baseline/BivariateNoise_B.png}"
  "DATA{Data/Baseline/BivariateNoise_C.png}"
  "DATA{Data/Baseline/BivariateNoise_D.png}")

paraview_add_client_tests(
  LOAD_PLUGIN "BivariateRepresentations"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS BivariateNoiseRepresentation.xml)
