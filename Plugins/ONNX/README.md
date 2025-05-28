# ONNX Plugin

This plugins add a filter to perform AI inference: **ONNX Predict**.

*ONNX* is a community project supported by the Linux Foundation, working toward
interoperability of AI tools. See more at <https://onnx.ai/about.html>.

## Usage

To use this filter, you will need to provide two files:
- an ONNX model file
- a Json file describing the input and output parameters

Expected Json format:
```json
 {
 "Version": {
   "Major": int,
   "Minor": int,
   "Patch": int
 },
 "Input": [
   {
     "Name": str,
     "Min": double,
     "Max": double,
     "Default": double
   },
   ...
   ,
   { "Name": str
     "Min": double,
     "Max": double,
     <opt>"IsTime": bool,
     <opt>"NumSteps": int
   }
 ],
 "Output": {
   "OnCellData": bool,
   "Dimension": int >= 1,
 }
```

The `Inputs` array list input parameters: those are bounded numerical values.
The `Output` describe how to associate the resulting array with the data.
It can be Cell or Point data with a dimension greater or equal to 1.

## Build

To use ONNX with a local build of Paraview, you will have to install an ONNX runtime
for C++ API https://onnxruntime.ai/

Note that due to some issue in the ONNX CMake integration with ONNX runtime 1.22.0 on linux
you will need to:
- create a "lib64" link to the "lib" directory next to it. <https://github.com/microsoft/onnxruntime/issues/25242>
- move the lib/*.h files under an "onnxruntime" subdirectory (to create)
  <https://github.com/microsoft/onnxruntime/issues/25279>
