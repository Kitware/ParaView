# This plugin bring the ParaViewWeb WebGL exporter to regular ParaView
# as a plugin.
# That plugin will behave as any other ParaView exporter (VRML, X3D, POV...)
# but will generate several types of files. The main one is the scene graph
# description define as a JSON object with all the corresponding binary+base64
# pieces that come along with it. But also with it come a single standalone HTML
# file that can directly be used to see the data in a browser without any plugin.
#
# This code base should be cleaned up to follow VTK standard and even be
# integrated into VTK itself. But for now it is provided as is.

SET(PLUGIN_NAME WebGLExporter)
SET(PLUGIN_VERSION "1.0")

INCLUDE_DIRECTORIES(
    ${PARAVIEW_INCLUDE_DIRS}
    ${VTK_INCLUDE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
  )

# ----------------------------------------------------------------------------
# Generate C++ header file that will embed the JavaScript content
# ----------------------------------------------------------------------------

SET(JAVASCRIPT_SRCS
  webglRenderer
  glMatrix
)

foreach(file ${JAVASCRIPT_SRCS})
  set(src ${CMAKE_CURRENT_SOURCE_DIR}/${file}.js)
  set(res ${CMAKE_CURRENT_BINARY_DIR}/${file}.h)
  add_custom_command(
    OUTPUT ${res}
    DEPENDS ${src}
    COMMAND ${VTK_ENCODESTRING_EXE}
    ARGS ${res} ${src} ${file}
    )
  set(EXTRA_SRCS ${EXTRA_SRCS} ${res})
endforeach(file)

# ----------------------------------------------------------------------------
# Generate ParaView plugin
# ----------------------------------------------------------------------------

ADD_PARAVIEW_PLUGIN(WebGLExporter "1.0"
   SERVER_MANAGER_XML
      WebglExporter.xml
   SERVER_MANAGER_SOURCES
      vtkPVWebGLExporter.cxx
      vtkWebGLDataSet.cxx
      vtkWebGLExporter.cxx
      vtkWebGLObject.cxx
      vtkWebGLPolyData.cxx
      vtkWebGLWidget.cxx
   SOURCES
      md5.cxx
      ${EXTRA_SRCS}
)
