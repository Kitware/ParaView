PipelineModule illustrates creating a VTK pipeline in a proxy and setting up a GUI module for it. The proxy is vtkSMShrunkContoursProxy. It illustrates how to create a special vtkSMSourceProxy which builds a pipeline. To provide access to this filter pipeline through GUI, ModuleInterface must be defined (in modules.xml). Environment variable PV_INTERFACE_PATH must be set to the directory containing the modules.xml, so that it will be parsed when ParaView is loaded.

1.) Set environment variable PV_INTERFACE_PATH to the directory containing modules.xml
2.) Start ParaView, a new filter by the name "Shrunk Contours" will be available.


