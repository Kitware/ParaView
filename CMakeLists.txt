
PROJECT(ParaQ)

# prevent runtime problems on Unix/Mac when Qt & VTK use
# separate instances of these libraries
# this requires Qt to be configured to use the 
# system libs as well (which is the default)
SET(VTK_USE_SYSTEM_PNG ON CACHE BOOL "Use system png")
SET(VTK_USE_SYSTEM_JPEG ON CACHE BOOL "Use system jpeg")
SET(VTK_USE_SYSTEM_FREETYPE ON CACHE BOOL "Use system freetype")
# ParaView/Utilities/hdf5/ doesn't build using system zlib
#SET(VTK_USE_SYSTEM_ZLIB ON CACHE BOOL "Use system zlib")

# turn off building ParaView Tk based GUI
SET(PARAVIEW_BUILD_GUI OFF CACHE BOOL "Build end user applications (ParaView, pvclient, pvbatch)")

SUBDIRS(
  ParaView
  Client
)

