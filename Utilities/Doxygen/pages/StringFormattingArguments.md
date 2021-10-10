String Formatting Arguments       {#StringFormattingArguments}
=====================

This page documents string formatting arguments that affect ParaView behavior at
runtime. These arguments can be used to format formattable strings.

The class that is responsible for handling the pushing/popping of scopes (set of arguments) is `vtkPVStringFormatter`.
`vtkPVStringFormatter` uses the `{fmt}` library to format strings that utilize named arguments.

The following arguments are part of the `initial argument scope`. This scope is
pushed and popped inside `vtkInitializerHelper`, therefore the values of those
arguments do NOT change at runtime. The arguments of this scope can be used in all formattable strings.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{ENV_username}` | std::string | The computer's username. Extracted from vtksys::SystemInformation.
`{username}` | std::string | The computer's username. Extracted from vtksys::SystemInformation.
`{ENV_hostname}` | std::string | The computer's hostname. Extracted from vtksys::SystemInformation.
`{hostname}` | std::string | The computer's hostname. Extracted from vtksys::SystemInformation.
`{ENV_os}` | std::string | The computer's operating system. Extracted from vtksys::SystemInformation.
`{os}` | std::string | The computer's operating system. Extracted from vtksys::SystemInformation.
`{GLOBAL_date}` | std::chrono::time_point | The date that ParaView started its execution. Extracted from std::chrono::system_clock::now(). For further formatting options see [fmt/chrono.h](https://gitlab.kitware.com/third-party/fmt/-/blob/master/include/fmt/chrono.h).
`{date}` | std::chrono::time_point | The date that ParaView started its execution. Extracted from std::chrono::system_clock::now(). For further formatting options see [fmt/chrono.h](https://gitlab.kitware.com/third-party/fmt/-/blob/master/include/fmt/chrono.h).
`{GLOBAL_appname}` | std::string | The application name. Extracted from vtkInitializationHelper::ApplicationName.
`{appname}` | std::string | The application name. Extracted from vtkInitializationHelper::ApplicationName.
`{GLOBAL_appversion}` | std::string | The application version. Extracted from PARAVIEW_VERSION_FULL.
`{appversion}` | std::string | The application version. Extracted from PARAVIEW_VERSION_FULL.

The following arguments are part of a scope that is used in `Views`. `vtkPVXYChartView` and `vtkPVPlotMatrixView`
are responsible for pushing and popping the scope.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{VIEW_time}` | double | The time of the VIEW.
`{time}` | double | The time of the VIEW.

The following arguments are part of a scope that is used in `Views` also. `vtkPVBagPlotMatrixView` is responsible
for pushing and popping the scope. `vtkPVBagPlotMatrixView` can also use the scope that is pushed by
its superclass, `vtkPVPlotMatrixView`.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{VIEW_variance}` | int | The explained variance. Extracted from `vtkPVBagPlotMatrixRepresentation`.
`{variance}` | int | The explained variance. Extracted from `vtkPVBagPlotMatrixRepresentation`.

The following arguments are part of a scope that is used in `vtkTimeToTextConvertor`. This class is responsible
for pushing and popping the scope.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{TEXT_time}` | double | The time. Extracted from vtkDataObject::DATA_TIME_STEP() / vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP().
`{time}` | double | The time. Extracted from vtkDataObject::DATA_TIME_STEP() / vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP().

The following arguments are part of a scope that is used in `extract_writers`. `vtkSMExtractsController` is
responsible for pushing and popping the scope.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{EXTRACT_timestep}` | int | The time-step of the extraction. Extracted from GetTimeStep().
`{timestep}` | int | The time-step of the extraction. Extracted from GetTimeStep().
`{EXTRACT_time}` | double | The time of the extraction. Extracted from GetTime().
`{time}` | double | The time of the extraction. Extracted from GetTime().

The following arguments are part of a scope that is used also in `extract_writers`. `vtkSMImageExtractWriterProxy` is
responsible for pushing and popping the scope. `vtkSMImageExtractWriterProxy` can also utilize the scope pushed by
`vtkSMExtractsController`.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{EXTRACT_camera}` | std::string | The camera parameters. Extracted from GetActiveCamera().
`{camera}` | std::string | The camera parameters. Extracted from GetActiveCamera().

The following arguments are part of a scope that is used in `PythonAnnotation`. `vtkPythonAnnotationFilter` is
responsible for pushing and popping the scope.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{ANNOTATE_timevalue}` | double | The time-value of the information. Extracted from vtkDataObject::DATA_TIME_STEP().
`{timestep}` | double | The time-value of the information. Extracted from vtkDataObject::DATA_TIME_STEP().
`{ANNOTATE_timesteps}` | std::vector<double> | The time-step values. Extracted from vtkStreamingDemandDrivenPipeline::TIME_STEPS().
`{timesteps}` | std::vector<double> | The time-step values. Extracted from vtkStreamingDemandDrivenPipeline::TIME_STEPS().
`{ANNOTATE_timerange}` | std::vector<double> | The time-range of time-steps. Extracted from vtkStreamingDemandDrivenPipeline::TIME_RANGE().
`{timerange}` | std::vector<double> | The time-range of time-steps. Extracted from vtkStreamingDemandDrivenPipeline::TIME_RANGE().
`{ANNOTATE_timeindex}` | int | The index of the time-steps element which is equal to time-value.
`{timeindex}` | int | The index of the time-steps element which is equal to time-value.

The following arguments are part of a scope that is used in `PythonCalculator`. `vtkPythonCalculator` is
responsible for pushing and popping the scope.

Argument | Type | Description
---------|------|---------------------------------------------------------
`{CALCULATOR_timevalue}` | double | The time-value of the information. Extracted from vtkDataObject::DATA_TIME_STEP().
`{timevalue}` | double | The time-value of the information. Extracted from vtkDataObject::DATA_TIME_STEP().
`{CALCULATOR_timeindex}` | int | The index of the time-steps element which is equal to time-value. time-steps are extracted from vtkStreamingDemandDrivenPipeline::TIME_STEPS().
`{timeindex}` | int | The index of the time-steps element which is equal to time-value. time-steps are extracted from vtkStreamingDemandDrivenPipeline::TIME_STEPS().

All the aforementioned arguments can be used in appropriate ParaView use cases to format formattable strings.
If a user wants to see which are the available arguments that he can use, he can try to type a wrong argument in the
formattable field, e.g. `{test123}`. In output messages of ParaView, the user will see a message like below:

```
ERROR: In ~/ParaView/VTKExtensions/Core/vtkPVStringFormatter.cxx, line 73
vtkPVStringFormatter (0x55984453b2a0):
Invalid format specified '{test123}'
Details: argument not found

This argument scope includes the following arguments:
    Name: ENV_username
    Name: ENV_hostname
    Name: ENV_os
    Name: GLOBAL_date
    Name: GLOBAL_appname
    Name: GLOBAL_appversion
    Name: time
    Name: TEXT_time
```
