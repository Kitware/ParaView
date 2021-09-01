#String substitutions using {fmt}

The goal of MR is the standardization of string substitutions as described by @utkarsh.ayachit in https://gitlab.kitware.com/paraview/paraview/-/issues/20547.

To implement the described functionality, the ``{fmt}`` library has been utilized. ``{fmt}`` had to be modified (see https://github.com/fmtlib/fmt/pull/2432) to support the desired functionality.

As described in https://gitlab.kitware.com/paraview/paraview/-/issues/20547, a stack-based approach of argument-scopes has been implemented. ``vtkPVStringFormatter`` is responsible for pushing and popping argument scopes, and formatting strings formattable strings.
* ``vtkInitializerHelper`` is responsible for pushing and popping the initial argument scope of the scope stack. This scope includes the following arguments which do **NOT** change during the execution of ParaView:
  * ``{ENV_username} == {username}``: e.g. firstname.lastname (extracted from vtksys::SystemInformation)
  * ``{ENV_hostname} == {hostname}``: e.g. kwintern-1 (extracted from vtksys::SystemInformation)
  * ``{ENV_os} == {os}``: e.g. LINUX (extracted from vtksys::SystemInformation)
  * ``{GLOBAL_date} == {date}``: e.g. 2021-08-02 11:55:57 (extracted from std::chrono::system_clock::now())
    * For further formatting options see [fmt/chrono.h](https://gitlab.kitware.com/third-party/fmt/-/blob/master/include/fmt/chrono.h), e.g. ``{GLOBAL_date:%a %m %b %Y %I:%M:%S %p %Z}`` becomes Mon 08 Aug 2021 11:55:57 AM EDT
  * ``{GLOBAL_appname} == {appname}``: e.g. ParaView (extracted from vtkInitializationHelper::ApplicationName)
  * ``{GLOBAL_appversion} == {appversion}``: e.g. 5.9.1-1459-g3abc5dec2d (extracted from PARAVIEW_VERSION_FULL)
* You can push a new scope of arguments to the scope stack as follows ```PushScope(fmt:arg("time", time), fmt:arg("timestep", timestep))```.
  * When a new scope is pushed, it will have as a baseline the top scope of the stack.
  * The argument names that an argument scope has must be unique.
* You can push a new **named** scope of arguments to the scope stack as follows ```PushScope("EXTRACT", fmt:arg("time", time), fmt:arg("timestep", timestep))```
  * This scope push will create the following arguments `{EXTRACT_time}`, `{time}`, `{EXTRACT_timestep}`, `{timestep}`.
* A scope of arguments can be popped from the scope stack as follows ``` PopScope()```
* A string can be formatted using ``` Format(const char* formattableString)```,
  * The top scope of the scope stack will always be used for formatting.
  * If the format fails the then the names of the argument scope are printed with the output error messages of ParaView.
* You can also automate the push/pop of scopes using the macros ``PV_STRING_FORMATTER_SCOPE`` and ``PV_STRING_FORMATTER_NAMED_SCOPE`` which are used in the same way as ``PushScope``, and the ``PopScope`` is called when the statement gets out of the code scope.

Currently, the following four modules utilize ``vtkPVStringFormatter``:
* ``Extract_writers``
* ``Views``
* ``Annotate Time (vtkTextToTimeFilter)``
* ``EnvironmentAnnotation (vtkEnvironmentAnnotationFilter)``
* ``PythonAnnotation (vtkPythonAnnotationFilter)``
* ``PythonCalulator (vtkPythonCalculator)``

``Extract_writers`` include the newly added extract_generators that are listed in the ``Extractors`` drop-down menu of ParaView (e.g. CSV, PNG, JPG, VTP). All the extractors used to have the following arguments ``%ts`` (int), ``%t`` (double) and (optionally if the extractor is an image) ``%cm`` (string). It should be noted that ``{fmt}`` does not support specified precision for integers, so {timestep:.6d} does not work, but {timestep:06d} does work.
* ``vtkSMExtractsController`` is responsible for inserting the scope of the following arguments ``{time} == {EXTRACT_time}`` and ``{timestep} = {EXTRACT_timestep}``.
* If an image is extracted, then inside the ``vtkSMImageExtractWriterProxy`` before calling ``vtkSMExtractWriterProxy::GenerateImageExtractsFileName`` a new scope is added that includes the ``{EXTRACT_camera} == {camera}`` argument.
  * It should be noted that the GenerateFilename functions of ``vtkSMExtractWriterProxy`` (``GenerateDataExtractsFileName``, ``GenerateImageExtractsFileName``, ``GenerateExtractsFileName``) have been merge into the function ``GenerateExtractsFileName``.
* ``vtkSMExtractWriterProxy::GenerateExtractsFileName`` is responsible for formatting the string.
* If the old format is used, it is replaced with the new one and prints a warning.

``Views`` include all the classes that utilize the ``GetFormattedTitle()`` function of ``vtkPVContextView``. ``vtkPVContextView`` used to use ``${TIME}`` and ``vtkPVBagPlotMatrixView`` used to use additionally ``${VARIANCE}``.
* ``vtkPVPlotMatrixView`` and ``vtkPVXYChartView`` that use the ``GetFormattedTitle()`` push a scope with ``{time} == {VIEW_time}``.
* ``vtkPVBagPlotMatrixView`` is overriding the ``Render`` function to push a scope that includes ``{VIEW_variance} == {variance}`` on top of the scope of that it inherits from ``vtkPVPlotMatrixView``.
* If the old format is used, it is replaced with the new one and prints a warning.

``Annotate Time`` used to use ``%f`` to substitute time, but now is using a scope that includes ``{time} == {TEXT_time}``. If the old format is used, it is replaced with the new one and prints a warning.

``EnvironmentAnnotation`` used to call hard-coded pieces of code to get ``GLOBAL`` and ``ENV`` arguments. Now it's using the ``vtkPVStringFormatter`` since these values have already been calculated.
* It should be noted that the date that is printed from ``EnvironmentAnnotation`` is ``{GLOBAL_date}`` therefore it does not change at every call of the filter.

``PythonAnnotation`` used to use ``time_value``, ``time_steps``, ``time_range``, and ``time_index`` in the provided expression. Now it can also use ``{timevalue} == {ANNOTATE_timevalue}``, ``{timesteps} == {ANNOTATE_timesteps}``, ``{timerange} == {ANNOTATE_timerange}`` and ``{timeindex} == {ANNOTATE_timeindex}``.

``PythonCalulator`` used to use ``time_value``, and ``time_index`` in the provided expression. Now it can also use ``{timevalue} == {ANNOTATE_timevalue}``, and ``{timeindex} == {ANNOTATE_timeindex}``.
