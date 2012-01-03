# look at each path and try to find ifconsol.lib

set(PATH "$ENV{PATH}")
set(LIB "$ENV{LIB}")
set(INCLUDE "$ENV{INCLUDE}")

foreach(dir ${PATH})
  file(TO_CMAKE_PATH "${dir}" dir)
  if(EXISTS "${dir}/ifort.exe")
    file(APPEND output.cmake "set(intel_ifort_path \"${dir}\")\n")
    break()
  endif()
endforeach()

foreach(dir ${LIB})
  file(TO_CMAKE_PATH "${dir}" dir)
  if(EXISTS "${dir}/ifconsol.lib")
    file(APPEND output.cmake "set(intel_lib_dir \"${dir}\")\n")
    break()
  endif()
endforeach()

foreach(dir ${INCLUDE})
  file(TO_CMAKE_PATH "${dir}" dir)
  if(EXISTS "${dir}/ifcore.f90")
    file(APPEND output.cmake "set(intel_include_dir \"${dir}\")\n")
    break()
  endif()
endforeach()
