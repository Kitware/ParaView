
set(exe ${PointSprite_EXAMPLE_EXECUTABLE_PREFIX}PointSpriteDemo)

add_executable(${exe} PointSpriteDemo.cxx)

target_link_libraries(${exe} CSCS_PointSprite_Rendering vtkIO)

install(TARGETS ${exe} 
    RUNTIME DESTINATION ${PROJECT_BINARY_DIR}
)
