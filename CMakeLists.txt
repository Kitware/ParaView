PROJECT(ParaViewComplete)
SUBDIRS(VTK ParaView)

SET (LIBRARY_OUTPUT_PATH ${ParaViewComplete_BINARY_DIR}/bin/ CACHE PATH 
"Single output directory for building all libraries.")
SET (EXECUTABLE_OUTPUT_PATH ${ParaViewComplete_BINARY_DIR}/bin/ CACHE PATH 
"Single output directory for building all executables.")

# force these to be ON by default as they are required by ParaView
OPTION (VTK_USE_HYBRID "Build the hybrid directory classes" ON)
OPTION (VTK_USE_PARALLEL "Build the parallel directory classes" ON)
OPTION (VTK_WRAP_TCL "wrap classes into the TCL intepreted language" ON)
