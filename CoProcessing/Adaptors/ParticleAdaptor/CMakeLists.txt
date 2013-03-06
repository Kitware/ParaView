cmake_minimum_required(VERSION 2.8.8)
project(ParticleAdaptor)

find_package(ParaView REQUIRED COMPONENTS vtkPVCatalyst)
include("${PARAVIEW_USE_FILE}")

add_library(ParticleAdaptor
  ParticleAdaptor.cxx
  vtkParticlePipeline.cxx)

target_link_libraries(ParticleAdaptor vtkPVCatalyst)
