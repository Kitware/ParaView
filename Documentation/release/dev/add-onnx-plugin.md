## Add ONNX plugin

ParaView now has an ONNX plugin to compute inference from an ONNX model,
thanks to the **ONNX Predict** filter.

ONNX is a community project supported by the Linux Foundation, working toward
interoperability of AI tools <https://onnx.ai/about.html>

### Developer notes
To use ONNX with a local build of Paraview, you will have to install an ONNX runtime
for C++ API https://onnxruntime.ai/

Note that due to some issue in the ONNX CMake integration with ONNX runtime 1.22.0 on linux
you will need to:
- create a "lib64" link to the "lib" directory next to it. <https://github.com/microsoft/onnxruntime/issues/25242>
- move the lib/*.h files under an "onnxruntime" subdirectory (to create)
  <https://github.com/microsoft/onnxruntime/issues/25279>
