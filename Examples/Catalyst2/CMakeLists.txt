if (PARAVIEW_CONTRACT_FILE_URLS_CATALYST)
  include(ParaViewContractTesting)

  set(extra_cmake_args)
  if (PARAVIEW_CONTRACT_CATALYST_REVISION)
    list(APPEND extra_cmake_args
      "-DCATALYST_EXAMPLES_CONTRACT_TAG:STRING=${PARAVIEW_CONTRACT_CATALYST_REVISION}")
  endif ()

  list(REMOVE_DUPLICATES PARAVIEW_CONTRACT_FILE_URLS_CATALYST)
  foreach (contract_url IN LISTS PARAVIEW_CONTRACT_FILE_URLS_CATALYST)
    paraview_contract_test(
      TEST_FILE_URL "${contract_url}"
      EXTRA_CMAKE_ARGUMENTS ${extra_cmake_args})
  endforeach ()
endif ()
