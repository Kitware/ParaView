ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{Data/Baseline/BivariateNoise_A.png}"
  "DATA{Data/Baseline/BivariateNoise_B.png}"
  "DATA{Data/Baseline/BivariateNoise_B_1.png}"
  "DATA{Data/Baseline/BivariateNoise_B_2.png}"
  "DATA{Data/Baseline/BivariateNoise_B_3.png}"
  "DATA{Data/Baseline/BivariateNoise_C.png}"
  "DATA{Data/Baseline/BivariateNoise_C_1.png}"
  "DATA{Data/Baseline/BivariateNoise_C_2.png}"
  "DATA{Data/Baseline/BivariateNoise_C_3.png}"
  "DATA{Data/Baseline/BivariateNoise_C_4.png}"
  "DATA{Data/Baseline/BivariateNoise_D.png}"
  "DATA{Data/Baseline/BivariateTexture_XX.png}"
  "DATA{Data/Baseline/BivariateTexture_XY.png}"
  "DATA{Data/Baseline/BivariateTexture_YX.png}"
  "DATA{Data/Baseline/BivariateTexture_YY.png}"
  "DATA{Data/Baseline/BivariateTexture_SurfaceRepr.png}")

set(tests
  "BivariateNoiseRepresentation.xml"
  "BivariateTextureRepresentation.xml"
)

paraview_add_client_tests(
  LOAD_PLUGIN "BivariateRepresentations"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS ${tests}
  # The test lists OpenGL features, some containing the keyword error. This
  # result in false positive errors.
  SMTESTING_ALLOW_ERRORS
)
