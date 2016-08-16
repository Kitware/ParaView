cmake_minimum_required(VERSION 3.3)
project(ParticleAdaptor)

find_package(ParaView REQUIRED COMPONENTS vtkPVCatalyst)
include("${PARAVIEW_USE_FILE}")

add_library(ParticleAdaptor
  ParticleAdaptor.cxx
  vtkParticlePipeline.cxx)

target_link_libraries(ParticleAdaptor vtkPVCatalyst)
