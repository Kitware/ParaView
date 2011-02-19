
set(exe ${PointSprite_EXAMPLE_EXECUTABLE_PREFIX}PointSpriteDemo)

add_executable(${exe} PointSpriteDemo.cxx)

target_link_libraries(${exe} PointSprite_Rendering vtkIO)
