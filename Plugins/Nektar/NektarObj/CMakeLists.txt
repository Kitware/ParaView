
project(nektarobj)


if(NOT WIN32)
# need some try compiles
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

add_library(nektarobj STATIC nektarObject.C)
