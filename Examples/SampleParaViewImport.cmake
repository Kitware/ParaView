######################################################################
# in the example replace Sample with the name of your module
# Sample_SOURCE_DIR will already be set by ParaView
######################################################################


# Define the source files that should be built
SET (Sample_SRCS 
  ${Sample_SOURCE_DIR}/vtkFUBAR.cxx
  ${Sample_SOURCE_DIR}/vtkSuperFUBAR.cxx
  )

# Define sources that should be built and wrapped in the client server
SET (Sample_WRAPPED_SRCS 
  ${Sample_SOURCE_DIR}/vtkThreadedFUBAR.cxx
  )

# Add in any library directories for libraries that Sample requires
#PARAVIEW_EXTRA_LINK_DIRECTORIES("${SOME_OTHER_LIBRARY_DIRS}")

# set any include directories this module needs
INCLUDE_DIRECTORIES(${Sample_SOURCE_DIR})

# invoke this macro to add link libraries to PV
PARAVIEW_LINK_LIBRARIES("${Sample_LIBS}")

# invoke this macro to add the sources to paraview and wrap them for the
# client server 
PARAVIEW_INCLUDE_WRAPPED_SOURCES("${Sample_WRAPPED_SRCS}")

# invoke this macro to add the sources to the build (but not wrap them into
# the client server
PARAVIEW_INCLUDE_SOURCES("${Sample_SRCS}")

# invoke this macro to add sources into the client and also wrap them into Tcl
#PARAVIEW_INCLUDE_CLIENT_SOURCES("${Sample_GUI_SRCS}")
