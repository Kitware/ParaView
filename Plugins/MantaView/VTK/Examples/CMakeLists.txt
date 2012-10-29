INCLUDE_DIRECTORIES(../)

ADD_EXECUTABLE(manta_cylinder MACOSX_BUNDLE manta_cylinder.cxx)
TARGET_LINK_LIBRARIES(manta_cylinder vtkManta)

ADD_EXECUTABLE(manta_rendered_file MACOSX_BUNDLE manta_rendered_file.cxx)
TARGET_LINK_LIBRARIES(manta_rendered_file vtkManta)

ADD_EXECUTABLE(manta_rendered_water MACOSX_BUNDLE manta_rendered_water.cxx)
TARGET_LINK_LIBRARIES(manta_rendered_water vtkManta)

