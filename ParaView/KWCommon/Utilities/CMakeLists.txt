ADD_EXECUTABLE(ConvertImage ConvertImage.cxx)

TARGET_LINK_LIBRARIES(ConvertImage
    vtkCommon
    vtkFiltering
    vtkImaging
    vtkIO
    vtkpng
    vtkzlib
)
