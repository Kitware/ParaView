add_subdirectory(Cxx)
add_subdirectory(Legacy)

#---------------------------------------------------------------------
set(simple_scripts
  ValidateChangingInput.py
  LiveOnly.py)

paraview_add_test(
  PREFIX "Catalyst::WaveletMiniApp"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21 --script-version 2 -s
  TEST_SCRIPTS ${simple_scripts})
paraview_add_test_mpi(
  PREFIX "Catalyst::MPI::WaveletMiniApp"
  ARGS   "$<TARGET_FILE:ParaView::pvbatch>" -sym -m paraview.demos.wavelet_miniapp -t 20 --size 21 --script-version 2 -s
  TEST_SCRIPTS ${simple_scripts})
