IF(VTK_ZLIB_INCLUDE_DIRS)
  INCLUDE_DIRECTORIES(${VTK_ZLIB_INCLUDE_DIRS})

  ADD_EXECUTABLE(ConvertImage ConvertImage.cxx)

  TARGET_LINK_LIBRARIES(ConvertImage
    vtkCommon
    vtkFiltering
    vtkImaging
    vtkIO
    )
ENDIF(VTK_ZLIB_INCLUDE_DIRS)

