set(catalyst_example_names
  paraview-CUnstructuredGrid
  paraview-CxxImageData
  paraview-CxxMultiChannelInput
  paraview-CxxMultimesh
  paraview-CxxOverlappingAMR
  paraview-CxxPolygonalWithAttributes
  paraview-CxxPolyhedra
  paraview-CxxSteering
  paraview-CxxUnstructuredGrid
  paraview-Fortran90ImageData
)

if ("$ENV{CMAKE_CONFIGURATION}" MATCHES "python")
  list(APPEND catalyst_example_names
    paraview-PythonImageData
    paraview-PythonSteering)
endif ()

################################################################################
# Set the revision to build as contract tests here. Commit hashes, tags, and
# branch names are supported.
################################################################################
set(catalyst_examples_revision "ParaViewCI-25.07")
################################################################################

set(paraview_contract_file_urls)
foreach (catalyst_example_name IN LISTS catalyst_example_names)
  list(APPEND paraview_contract_file_urls
    "https://gitlab.kitware.com/paraview/catalyst-examples/-/raw/${catalyst_examples_revision}/CMake/contract-tests/${catalyst_example_name}.cmake")
endforeach ()

string(LENGTH "${catalyst_examples_revision}" catalyst_examples_revision_length)
if (catalyst_examples_revision MATCHES "^[0-9a-f]*$" AND
    catalyst_examples_revision_length EQUAL "40")
  # specific revision
elseif (catalyst_examples_revision MATCHES "^v[0-9.]+$" OR
        catalyst_examples_revision MATCHES "^ParaViewCI-")
  # tag name
else () # assume a branch name
  string(PREPEND catalyst_examples_revision "origin/")
endif ()
set(PARAVIEW_CONTRACT_CATALYST_REVISION "${catalyst_examples_revision}" CACHE STRING "")
set(PARAVIEW_CONTRACT_FILE_URLS_CATALYST "${paraview_contract_file_urls}" CACHE STRING "")
