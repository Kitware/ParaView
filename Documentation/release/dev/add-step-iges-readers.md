## Add support for reading STEP and IGES files

ParaView can now read .step, .stp, .iges files using vtkPVOCCTReader which is a subclass of vtkOCCTReader requires
OpenCascade installed. This reader can be enabled using the PARAVIEW_ENABLE_OCCT option.
