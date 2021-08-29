ParaView 5.10.0 Release Notes
=============================

Major changes made since ParaView 5.9.1 are listed in this document. The full list of issues addressed by this release is available [here](https://gitlab.kitware.com/paraview/paraview/-/issues?scope=all&state=closed&milestone_title=5.10%20(Fall%202021)).

* [New features](#new-features)
* [Rendering enhancements](#rendering-enhancements)
* [Plugin updates](#plugin-updates)
* [Filter changes](#filter-changes)
* [Readers, writers, and filters changes](#readers-writers-and-filters-changes)
* [Interface improvements](#interface-improvements)
* [Python scripting improvements](#python-scripting-improvements)
* [Virtual reality improvements](#virtual-reality-improvements)
* [Miscellaneous bug fixes](#miscellaneous-bug-fixes)
* [Catalyst](#catalyst)
* [Cinema](#cinema)
* [Developer notes](#developer-notes)

# New features

## macOS arm64 Binaries

ParaView is now tested on the macOS arm64 platform.

Due to this change, ParaView binaries are now available using the processor name according to the platform in the binary filename rather than a generic "32Bit" or "64Bit" indicator.

Linux: `x86_64`
macOS: `arm64` and `x86_64`
Windows: `AMD64`

## HPC benchmarks and validation suite

To make it easier to test and validate HPC builds, we have added a new package under the `paraview` Python package called `tests`. This package includes several modules that test and validate different aspects of the ParaView build.

The tests can be run as follows:

```python
# all tests
pvpython -m paraview.tests -o /tmp/resultsdir

# specific tests
pvpython -m paraview.tests.verify_eyedomelighting -o /tmp/eyedome.png
pvpython -m paraview.tests.basic_rendering -o /tmp/basic.png
```

Use the `--help` or `-h` command line argument for either the `paraview.tests`
package or individual test module to get list of additional options available.

The list is expected to grow over multiple releases. Suggestions to expand this validation
test suite are welcome.

## Command-line option parsing

The command-line options parsing code has been completely refactored. ParaView now uses [CLI11](https://github.com/CLIUtils/CLI11). This has implications for users and developers alike.

### Changes for users

There way to specify command line options is now more flexible. These can be provided as follows:

* `-a` : a single flag / boolean option
* `-f filename`: option with value, separated by space
* `--long`:  long flag / boolean option
* `--file filename`: long option with value separated using a space
* `--file=filename`: long option with value separated by equals

Note this is a subset of ways supported by CLI11 itself. This is because ParaView traditionally supported options of form `-long=value` i.e. `-` could be used as the prefix for long-named options. This is non-standard and now deprecated. Instead, one should add use `--` as the prefix for such options, e.g., `-url=...` becomes `--url=...`. Currently, this is done automatically to avoid disruption and a warning is raised. Since this conflicts with some of the other more flexible ways of specifying options in `CLI11`, we limit ourselves to the ways listed above until this legacy behavior is no longer supported.

The `--help` output for all ParaView executables is now better formatted. Options are grouped, making it easier to inspect related options together. Mutually exclusive options, deprecated options are clearly marked to minimize confusion. Also, in most terminals, the text width is automatically adjusted to the terminal width and text is wrapped to make it easier to read.

Several options supports overriding default values using environment variables. If the option is not specified on the command line, then that denoted environment variable will be used to fetch the value for that option (or flag).

### Changes for developers

`vtkPVOptions` and subclasses are deprecated. Instead of a single class that handled the parsing of the defining of command line flags/options, command line arguments, and then keep state for the flag/option selections, the new design uses two different classes. `vtkCLIOptions` handle the parsing (using CLI11), while singletons such as `vtkProcessModuleConfiguration`, `vtkRemotingCoreConfiguration`, `pqCoreConfiguration` maintain the state and also populate `vtkCLIOptions`  with supported flags/options. Custom applications can easily add their own `*Configuration` classes to populate `vtkCLIOptions` to custom options or override the default ParaView ones. If your custom code was simply checking user selections from `vtkPVOptions` or subclasses, change it to using the corresponding `*Configuration` singleton

## Consistent string substitutions and formatting across ParaView

The format of text strings used to reference certain quantities in ParaView have been standardized across sources, filters, and views. ParaView now uses the `{fmt}` library and associated syntax to reference particular quantities available in different contexts.

In all sources, filters, and views that make use of substitution strings, the following globally available text strings can be used to reference the information they represent:

* `{username}` (equivalently, `{ENV_username}`) - name of user logged in to machine on which ParaView is running
* `{hostname}` (equivalently, `{ENV_hostname}`) - name of host machine on which ParaView is running
* `{os}` (equivalently, `{ENV_os}`) - name of operating system on which ParaView is running
* `{date}` (equivalently, `{GLOBAL_date}`) - date and time at which ParaView was started.
* `{appname}` (equivalently, `{GLOBAL_appname}`) - name of the application, e.g ParaView. For ParaView-based applications, this will evaluate to the name of the application.
* `{appversion}` (equivalently, `{GLOBAL_appversion}`) - version of the application.

The actual information represented by the strings listed above is substituted into the string that is rendered, subject to [formatting modifiers](https://fmt.dev/latest/syntax.html#format-specification-mini-language) supported by the `{fmt}` library for strings, numbers, and dates.

These sources and filters use the new string formatting method:

* **Annotate Time** - To reference time, use `{time}`. The number formatting syntax from the `{fmt}` library are available to control the precision of the displayed time, e.g., `{time:3.2f}` limits the number of displayed digits after the decimal point to two.
* **Environment Annotation** - Formatting methods are used internally only - there are no exposed text fields to modify.
* Views - The view types **Bar Chart View**, **Bar Chart View (Comparative)**, **Box Chart View**, **Histogram View**, **Line Chart View**, **Line Chart View (Comparative)**, **Plot Matrix View**, **Point Chart View** and  **Quartile Chart View** all have `{time}` available in their **Chart Title** properties in addition to the globally available substitutions. The **Bag Chart View (Comparative)** views provided in the `BagPlotViewsAndFilters` plugin also defines a `{variance}` string substitution available for use in the **Chart Title** property.
* Extract writers - All extract writers listed in the *Extractors* menu have `{time}` (equivalently, `{EXTRACT_time}`) and an additional `{timestep}` (equivalently, `{EXTRACT_timestep}`) substitution that provides the current timestep index. In addition, image extractors have available a `{camera}` (equivalently, `{EXTRACT_camera}`) substitution for encoding camera information in the output **File Name** property.

>![string-formatter](img/5.10.0/string-formatter.png)
>
> New substitution string example in an **Annotate Time** filter.

Note: these string substitutions are not available in the **Python Annotation** filter. Access to time information remains unchanged in that filter.

## New stride property for animations

It is now possible to change the granularity of the animation using the **Stride** property in the animation view. This property only makes sense in the **Snap to Timesteps** and **Sequence** mode, i.e., when there is a fixed number of frames. The **Stride** property allows skipping a fixed number of frames to control how many frames should be displayed. For example, with a dataset having a time domain of `[0,1,2,3,4,5,6]` and when setting the stride to 2, only the times `[0,2,4,6]` will be used. The stride is taken into account when using the `Play`, `Go To Next Frame` or `Go To Previous Frame` buttons. A stride of 1 will act as the default behavior, i.e., no frames are skipped.

## New default view setting 'Empty'

The **Default View Type** advanced property under "General" settings/preferences tab now has an "Empty" option. This setting will open ParaView with an empty view that lets you select one of the available views.

## RealTime animation mode deprecated

The 'RealTime' animation mode is deprecated and will be removed in a future release. Rationale:

* ParaView does not aim do be a video editor.
* ParaView processes data and takes the time it needs so real-time replay is unlikely to happen as expected.

See the [discussion](https://discourse.paraview.org/t/paraview-time-refactoring/5506/6) leading to the decision to remove this animation mode.

# Rendering enhancements

## Render View background color

Render View (and related views) now have a property **UseColorPaletteForBackground** that indicates if the view-specific background overrides should be used or the view should simply use the global color palette to determine how the background is rendered. Previously, it was confusing to know which views were using the global color palette and which ones were using user overridden values for background color.

>![render-view-background-color](img/5.10.0/render-view-background-color.png)

In addition, instead of having separate boolean properties that are largely mutually exclusive, the view now adds a **BackgroundColorMode** property that lets the user choose how the background should be rendered. Supported values are "Single Color", "Gradient", "Texture" and "Skybox".

This change impacts the Python API. **UseGradientBackground**, **UseTexturedBackground** and **UseSkyboxBackground** properties have been replaced by **BackgroundColorMode** which is now an enumeration that lets you choose how to render the background. Also, unless compatibility <= 5.9 is specified, view specific background color changes will get ignored unless `UseColorPaletteForBackground` property is not
set to 0 explicitly.

## Simulate clear coat layer in the PBR representation

When using the PBR representation under the lighting panel, you can now add a second layer on top of the base layer. This is useful to simulate a coating on top of a material. This layer is dielectric (as opposed with metallic), and can be configured with various parameters. You can set the coat strength to control the presence of the coat layer (1.0 means strongest coating), the coat roughness and the coat color. You can also choose the index of refraction of the coat layer as well as the base layer. The more the index of refraction goes up, the more specular reflections there are.

You can also use a texture to normal map the coating. These parameters are supported by OSPRay pathtracer.

## Simulate anisotropic materials in the PBR representation

When using the PBR representation under the lighting panel, you can now set the anisotropy strength and anisotropy rotation for a material. The anisotropy strength controls the amount of light reflected along the anisotropy direction (i.e., the tangent). The anisotropy rotation rotates the tangent around the normal. Notice that the object must have normals and tangents defined to work. You can also use a texture to hold the anisotropy strength in the red channel, and the anisotropy rotation in the green channel. These parameters are supported by OSPRay pathtracer.

## Rotating skyboxes

You can now rotate the Skybox horizontally with Control + Left Click. This binding was previously set to rotate the view. You can still rotate the view with Left Click alone. This is useful when using "PBR" (physically-based rendering) shading and "Image Based Lighting" and you want to change the reflection without moving geometry in the scenery.

## **Text Source Representation** style options

The **Text Source Representation** used to represent how text sources are rendered has been updated to let you customize the borders and the background of the text. For the borders, it is now possible to draw round corners, choose the color as well as the thickness. A combo box allows you to define the visibility of the border (only on hover, always or never). For the text, you can now choose the background color and opacity and the right and top padding between the border and the text.

## Multicolumn and multiline support in equation text rendering

The rendering of equations using mathtext now supports multiline and multicolumn equations. You can define a new line by writing a new line in the text source, and you can define a new column by entering the character '|'. You can still write a '|' by escaping it with a backslash ('\|').

## View Property **WindowResizeNonInteractiveRenderDelay**

A new double property, **WindowResizeNonInteractiveRenderDelay**, on view proxies has been added that lets you set the time in seconds before doing a full-resolution render after the last WindowResize event. This allows the view to render interactively when resizing a window, improving rendering performance when using large data sets.

# Plugin updates

## GmshReader plugin deprecation

The `GmshReader` plugin is deprecated in favor of the `GmshIO` plugin.

# Filter changes

## **Calculator** filter rewrite

The **Calculator** filter implementation has a completely new backend and has been multhithreaded using VTK's symmetric multiprocessing tools. The implementation uses the [ExprTk](https://github.com/ArashPartow/exprtk) library for expression parsing and evaluation. The new implementation solves several bugs found in the previous calculator implementation, and it offers improved performance as well. Expression syntax is largely unchanged, but the expression for a dot product operation has changed from `A.B` to `dot(A,B)`. The previous implementation will be used for backwards compatibility when loading state files from ParaView 5.9.1 or earlier.

>![calculator-new](img/5.10.0/calculator-new.png)

## The Threshold filter can now threshold below or above a value

New thresholding methods have been added to the **Threshold** filter:

- **Between**: Keep cells with values between the lower and upper thresholds.
- **Below Lower Threshold**: Keep cells with values smaller than the lower threshold.
- **Above Upper Threshold**: Keep cells with values larger than the upper threshold.

Previously, it was possible to keep only cells with values falling between two values.

Accordingly, the property **ThresholdBetween** has been removed. Instead, the following three properties have been added: **LowerThreshold**, **UpperThreshold** and **ThresholdMethod**.

>![threshold-filter-changes](img/5.10.0/threshold-filter-changes.png)

## Changes to Extract Block filter

**Extract Block** filter now supports choosing blocks to extract using selector expressions. This makes the block selection more robust to changes to the input hierarchy than the previous selection implementation, which was based on block indices.

>|  |  |
>| ------- | ------- |
>|![extract-block-panel-assembly](img/5.10.0/extract-block-panel-assembly.png)|![extract-block-panel-selectors](img/5.10.0/extract-block-panel-selectors.png)|



## **Resample With Dataset** can work with partial arrays

Previously, **Resample With Dataset** would not pass partial arrays from composite dataset input. A new option, **Pass Partial Arrays** has been added. When on, partial arrays are sampled from the source data.

For all those blocks where a particular data array is missing, this filter uses `vtkMath::Nan()` for `double` and `float` arrays, and 0 for all other types of arrays (e.g., `int`, `char`, etc.)

## Histogram filter improvements

The **Histogram** filter has been enhanced in several ways:

* Values from hidden points or cells are no longer included in the histogram results.
* The data range used for automatic bin range values now excludes hidden points and cells.
* A new option, **Normalize**, makes the filter produce normalized histogram values for applications where knowing the fraction of values that fall within each bin is desired.

# Grid Connectivity has been removed

Filter named **Grid Connectivity** has been removed. The **Connectivity** filter should support a wider set of use cases than the **Grid Connectivity** filter and should be preferred.

## New **Convert To Point Cloud** filter

A new filter, **Convert To Point Cloud**, has been added. It let users convert any dataset into a `vtkPolyData` containing the same points as the inputs but with either a single poly vertex cell, many vertex cells or no cells.

>![convert-to-point-cloud](img/5.10.0/convert-to-point-cloud.png)

## New **Merge Time** filter

The **Merge Time** filter takes multiple temporal datasets as input and synchronizes them.

The output data is a multiblock dataset containing one block per input dataset. The output timesteps are the union (or the intersection) of each input's timestep list. Duplicate time values within an absolute or relative tolerance are removed.

## New **Merge Vector Components** filter

The **Merge Vector Components** filter takes three separate single-component data arrays and combines them into a single vector (three-component) data array in the output. [More](https://gitlab.kitware.com/paraview/paraview/-/issues/20491)

>![merge-vector-components](img/5.10.0/merge-vector-components.png)

## **Rotational Extrusion** filter has new axis selection property

The **Rotational Extrusion** filter has a new property named **Rotation Axis** to select the axis around which the extrusion shall be done. The default stays the z-axis.

# Readers, writers, and filters changes

## H.264-encoded MP4 video export on Windows

ParaView Windows binaries now offer export of H.264-encoded MP4 files. The frame rate and bit rate are both exposed as options.

## Ioss reader for Exodus files

ParaView now uses the Ioss library to read Exodus files. This new reader,`vtkIossReader`, was introduced in 5.9 as a plugin. In this release, ParaView now uses this new reader as the default reader for all Exodus files. The previous reader is still available and can be used in the UI by simply loading the **LegacyExodusReader** plugin. XML state files and Python scripts using the old Exodus reader explicitly will continue to work without any changes.

The Ioss reader has several advantages over the previous implementation. One of the major advantage is that it uses the modern `vtkPartitionedDataSetCollection` as the output data-type instead of `vtkMultiBlockDataSet`.

## Ioss reader for CGNS files

In additional to Exodus, Ioss reader now supports reading CGNS files as well. Note, the reader only supports a subset of CGNS files that are generated using the Ioss library and hence may not work for all CGNS files. The CGNS reader is still the preferred way for reading all CGNS files.

## Add OpenVDB Writer

ParaView now has a writer for [OpenVDB Format](https://www.openvdb.org/) files. The writer currently works with `vtkDataSets` and time-series data.

## Add OMF Reader

ParaView now has a reader for [Open Mining Format](https://omf.readthedocs.io/en/stable/index.html) files. You can specify which elements - point set, line set, surface, or volume - should be loaded.

## NIST SAVG file format reader

A reader for the SAVG file format, developed by the High Performance Computing and Visualization Group at NIST, an ASCII format supporting definition of primitives likes points, lines, and polygons.

## SEP reader enhancements

The SEP reader can now read up to `32` dimensions and output either 2D or 3D scalar fields. Be careful, though, as the proxy settings have changed: the default parameters for the scalar field may not necessarily correspond to the original one, leading to different output.

# Interface improvements

## Information Panel

The **Information Panel** has been redesigned to improve the way information is presented. Besides general layout improvements that make the panel more compact and consistent with other panels:

* When showing composite datasets, one can view information specific to a subtree by selecting that subtree in the panel. However, if the data changes, for example, due to animation, the selection was lost making it tedious to monitor data information changes for a specific subtree. This has been fixed.

* All numbers now use locale specific formatting. For example, for US-EN based locale, large number are formatted by placing commas. Memory used is shown in KB, MB, or GB as appropriate.

>![information-panel](img/5.10.0/information-panel.png)

## Find Data panel

The *Find Data* dialog has been replaced by *Find Data* panel which offers similar capabilities but is now a dockable panel.

The *Selection Display Inspector* been removed since the *Find Data* panel presents the same information under the *Selection Display* section.

>![find-data-panel](img/5.10.0/find-data-panel.png)

## Python Editor improvements

The following was added to the ParaView Python script editor:

* Auto save of opened buffers in swap files and possibility of recovery in case ParaView crashes
* Opening of multiple tabs in the editor
* Integration with ParaView's Python trace functionality in its own tab
* Integration with the Programmable Filter/Source/Annotation in a synchronized tab
* Improvements of the Undo/Redo feature within the editor
* A new entry in ParaView's "Edit" menu to access the editor directly
* Faster syntax highlighting
* Undo/redo capabilities
* More robust file saving

## New buttons ResetCameraClosest and ZoomClosestToData

Two new buttons "Reset Camera Closest" and "Zoom Closest To Data" has been added to the right of the existing "Reset Camera" and "Zoom To Data" buttons. These buttons reset the camera to maximize the screen occupancy occupation of the whole scene bounding box ("Reset Camera Closest") or of the active source bounding box ("Zoom Closest To Data").

>![zoom-closest](img/5.10.0/zoom-closest.png)

## Effect of modifier key reversed for saving, copying screenshots

When clicking the screenshot capture button, a simple click now saves the screenshot to a file and a click with a modifier key (Option or Ctrl) saves it to the clipboard, which is the opposite of what it was before.

When saving a screenshot to the clipboard, the view now blinks.

## OSPRay Material Editor

A new OSPRay material editor control panel has been added to control the OSPRay materials by selecting the "Material Editor" item in the "View" menu. The control panel is available when ParaView is compiled with `PARAVIEW_ENABLE_RAYTRACING` CMake option set to `ON` to enable the editor widget.

This editor enables the following:

* Load a material library using a `.json` file or create materials directly in the GUI
* Edit / Add / Remove properties on the materials
* Attach the selected material to the current selected source

## OSPRay Runtime Detection

ParaView will now detect whether OSPRay is supported at runtime and disable it
rather than acting as if it works. Of note is when using under macOS' Rosetta
translation support of `x86_64` binaries on `arm64` hardware.

## "ParaView Community Support" item in "Help" menu now links to discourse.paraview.org

The "ParaView Community Support" link in the "Help" menu will send you to the [ParaView Support Discourse[(https://discourse.paraview.org)] page to open a new topic or search for an existing one.

## Node editor plugin

It is now possible to visualize the pipeline as a node graph like other popular software like Blender. This is embedded in the **Node Editor** plugin and can be compiled by activating the `PARAVIEW_PLUGIN_ENABLE_NodeEditor` CMake option when compiling ParaView.

User controls are:

* Select Filters/Views by double-clicking their corresponding node labels (hold Ctrl to select multiple filters).
* Select output ports by double-clicking their corresponding port labels (hold Ctrl to select multiple output ports).
* Nodes are collapsed/expanded by right-clicking node labels.
* Selected output ports are set as the input of another filter by double-clicking the corresponding input port label.
* To remove all input connections, Ctrl+double-click on an input port.
* To toggle the visibility of an output port in the current active view, Shift+left-click the corresponding output port (Ctrl+Shift+left-click shows the output port exclusively)

# Python scripting improvements

## Improvements to 'LoadState' Python API

When loading `.pvsm` state files in Python using `LoadState` function, it was tricky to provide arguments to override data files used in the state file. The `LoadState` function has now been modified to enable you to specify filenames to override using a Python dictionary. The Python trace captures this new way of loading state files. Invocations of this function with previously used arguments is still supported and will continue to work.

Some examples:

```python
# use data files under a custom directory
LoadState(".....pvsm",
          data_directory="/...",
          restrict_to_data_directory=True)

# explicitly override files
LoadState(".....pvsm",
    filenames=[\
        {
            'name' : 'can.ex2',
            'FileName' : '/..../disk_out_ref.ex2',
        },
        {
            'name' : 'timeseries',
            'FileName' : [ '/..../sample_0.vtp',
                            '/..../sample_1.vtp',
                            '/..../sample_2.vtp',
                            '/..../sample_3.vtp',
                            '/..../sample_4.vtp',
                            '/..../sample_5.vtp',
                            '/..../sample_6.vtp',
                            '/..../sample_7.vtp',
                            '/..../sample_8.vtp',
                            '/..../sample_9.vtp',
                            '/..../sample_10.vtp']
        },
    ])
```

## Python initialization during import

To make creation of various proxies easier, ParaView defines classes for each known proxy type. These class types were immediately defined when `paraview.simple` was imported or a connection was initialized. Creation of these class types is now deferred until they are needed. This helps speed up ParaView Python initialization.

This change should be largely transparent to users except for those who were directly accessing proxy types from the `paraview.servermanager` as follows:

```python
# will no longer work
cls = servermanager.sources.__dict__[name]

# replace as follows (works in previous versions too)
cls = getattr(servermanager.sources, name)
```

## Fetching data to the client in Python

`paraview.simple` now provides a new way to fetch all data from the data server to  the client from a particular data producer. Using `paraview.simple.FetchData` users can fetch data from the data-server locally for custom processing.

Unlike `Fetch`, this new function does not bother applying any transformations to the data and hence provides a convenient way to simply access remote data.

Since this can cause large datasets to be delivered the client, this must be used with caution in HPC environments.

# Virtual reality improvements

## OpenVR Plugin improvements

Many updates to improve the OpenVR plugin support have been made in ParaView 5.10:

* Added a "Come to Me" button to bring other collaborators to your current location/scale/pose.

>![openvr-bring-collaborators-here](img/5.10.0/openvr-bring-collaborators-here.png)

* Fixed crop plane sync issues and a hang when in collaboration.
* Support desktop users in collaboration with an "Attach to View" option that makes the current view behave more like a VR view (shows avatars/crop planes etc).

>![openvr-desktop-collaboration](img/5.10.0/openvr-desktop-collaboration.png)

* Added a "Show VR View" option to show the VR view of the user when they are in VR. This is like the SteamVR option but is camera stabilized making it a better option for recording and sharing via video conferences.
* Broke parts of `vtkPVOpenVRHelper` into new classes named `vtkPVOpenVRExporter` and `vtkPVOpenVRWidgets` to break what was a large class into smaller classes and files to make the code a bit cleaner and more compartmentalized.
* Add Imago Image Support - added support for displaying images strings that are found in a dataset's cell data (optional, off by default).
* Fix thick crop stepping within collaboration.
* Update to new SteamVR Input API, which allows you to customize controller mappings for ParaView. Default bindings for VIVE, HP Motion, and Oculus Touch controllers are provided.
* Add an option controlling base station visibility.
* Re-enabled the option and GUI settings to set cell data value in VR.
* Adding points to a polypoint, spline, or polyline source when in collaboration now works even if the other collaborators do not have the same representation set as active.

## zSpace plugin

A new Microsoft Windows-only plugin named **zSpace** has been added. It adds a new view named **zSpaceView** that let a user interact with a zSpace device directly. This device is designed to work with Crystal Eyes stereo, in full screen or in a cave display. The zSpace glasses use a head tracking system that allow the user to look around 3D object by moving his head.

This plugin requires a [zSpace System Software >= 4.4.2](https://support.zspace.com/s/article/zSpace-System-Software-release-Required?language=en_US), and a SDK version >= 4.0.0.
It was tested on a zSpace 200 device but should be compatible with more recent devices as well.

# Miscellaneous bug fixes and updates

## Icons in macOS 11 style

The icons in macOS 11 Big Sur are in a new unified style, and ParaView's icon has been updated to conform to this style.

## ParaViewWeb

ParaViewWeb now supports `[unsigned] long long` data arrays when retrieving data information.

# Catalyst

## Add Conduit node IO and Catalyst replay executable

To assist in debugging in-situ pipelines, Catalyst now supports `conduit_node` I/O. The `params` argument to each invocation of `catalyst_initialize`, `catalyst_execute`, and `catalyst_finalize` can be written to disk. These can later be read back in using the `catalyst_replay` executable. Please see the corresponding [documentation](https://catalyst-in-situ.readthedocs.io/en/latest/catalyst_replay.html) for more information.

## Better Fortran Catalyst bindings

Fortran bindings to Catalyst APIs has been improved. There is now a `catalyst` and `catalyst_python` module available with namespaced APIs.

## `catalyst` module

* `coprocessorinitialize()` is now `catalyst_initialize()`
* `coprocessorfinalize()` is now `catalyst_finalize()`
* `requestdatadescription(step, time, need)` is now `need = catalyst_request_data_description(step, time)`
* `needtocreategrid(need)` is now `need = catalyst_need_to_create_grid()`
* `coprocess()` is now `catalyst_process()`

The `need` return values are of type `logical` rather than an integer.

## `catalyst_python` module

* `coprocessorinitializewithpython(filename, len)` is now `catalyst_initialize_with_python(filename)`
* `coprocessoraddpythonscript(filename, len)` is now `catalyst_add_python_script(filename)`

# Catalyst 2.0 Steering

Add steering capabilities to Catalyst 2, by implementing the `catalyst_results` method in ParaView Catalyst.

With steering, you are now able to change simulation parameters through the ParaView Catalyst GUI. See `CxxSteeringExample` in `Examples/Catalyst2` folder for live example of the feature.

>![catalyst-steering](img/5.10.0/catalyst-steering.png)

## Catalyst 2.0 TimeValue trigger

A new **TimeValue** trigger option for triggering output through Catalyst 2.0 has been added which is based on the amount of simulation time that has passed since the last output of the specified extract. This trigger option is more appropriate for simulation codes that have variable timestep lengths.

# Support for Polyhedral elements in Catalyst 2.0

Catalyst Adaptor API V2 now supports polyhedral elements.
This is done by adding support for polyhedral elements
in `vtkConduitSource`. A new example, `CxxPolyhedra`, demonstrates
how to use Conduit Mesh Blueprint to communicate information about
polyhedral elements.

## Added support for empty mesh Blueprint coming from Conduit to Catalyst

Catalyst 2.0 used to fail when the simulation code sent a mesh through Conduit with a full Conduit tree that matches the Mesh Blueprint but without any cells or points. This can happen, for example, with distributed data where some of the dataset may be empty on some ranks. When using `vtkUnstructedGrid`s in Catalyst 2.0 now checks the number of cells in the Conduit tree and returns an empty `vtkUnstructuredGrid` when needed.

[More](https://gitlab.kitware.com/paraview/paraview/-/issues/20833)

## Polygonal cell support in Conduit source

The goal of this merge is to add support for polygonal cells in the vtkConduitSource to allow for passing of cells of shape polygonal from a Conduit tree to VTK.

[More](https://gitlab.kitware.com/paraview/paraview/-/issues/20828)


# Cinema

## Extractor for recolorable images

This release reintroduces the experimental recolorable images generation capability. It is exposed using **Extractors**. A new experimental extractor called **Recolorable Image** is now available. The extractor is experimental and requires specific conditions to work.
# Developer notes

## Changes to `vtkPVDataInformation`

`vtkPVDataInformation` implementation has been greatly simplified.

`vtkPVDataInformation` no longer builds a complete composite data hierarchy information. Thus, `vtkPVCompositeDataInformation` is no longer populated and hence removed. This simplifies the logic in `vtkPVDataInformation` considerably.

`vtkPVDataInformation` now provides access to `vtkDataAssembly` representing the hierarchy for composite datasets. This can be used by application to support cases where information about the composite data hierarchy is needed. For `vtkPartitionedDataSetCollection`, which can other assemblies associated with it, `vtkPVDataInformation` also collects those.

For composite datasets, `vtkPVDataInformation` now gathers information about all unique leaf datatypes. This is useful for applications to determine exactly what type of datasets comprise a composite dataset.

`vtkPVTemporalDataInformation` is now simply a subclass of `vtkPVDataInformation`. This is possible since `vtkPVDataInformation` no longer includes `vtkPVCompositeDataInformation`.

## Example plugin for paraview_plugin_add_proxy

A new example plugin **AddPQProxy** has been added to demonstrate how to use the `paraview_plugin_add_proxy` CMake macros (`add_pqproxy` before ParaView 5.7.0).

## Improved documentation mechanism for ParaView plugins

Three new CMake options to the `paraview_add_plugin` function have been added:

 * **DOCUMENTATION_ADD_PATTERNS**: If specified, add patterns for the documentation files within `DOCUMENTATION_DIR` other than the default ones (i.e., `*.html`, `*.css`, `*.png`, `*.js` and `*.jpg`). This can be used to add new patterns (ex: `*.txt`) or even subdirectories (ex: `subDir/*.*`). Subdirectory hierarchy is kept so if you store all of your images in a `img/` sub directory and if your html file is at the root level of your documentation directory, then you should reference them using `<img src="img/my_image.png"/>` in the html file.

 * **DOCUMENTATION_TOC**: If specified, the function will use the given string to describe the table of content for the documentation. A TOC is divided into sections. Every section points to a specific file (`ref` keyword) that is accessed when double-clicked in the UI. A section that contains other sections can be folded into the UI. An example of such a string is:
```html
  <toc>
    <section title="Top level section title" ref="page1.html">
      <section title="Page Title 1" ref="page1.html"/>
      <section title="Sub section Title" ref="page2.html">
        <section title="Page Title 2" ref="page2.html"/>
        <section title="Page Title 3" ref="page3.html"/>
      </section>
    </section>
  </toc>
```

 - **DOCUMENTATION_DEPENDENCIES**: Targets that need to be built before actually building the documentation. This can be useful when a plugin relies on a third party documentation generator such as [Doxygen](https://www.doxygen.nl/).

See `Examples/Plugins/ElevationFilter` for an example of how to use these features.

## Javascript support for documentation in Paraview

It is possible to add a `.js` file to a plugins documentation and reference it into an `.html` file. However JS scripts will only be evaluated if `PARAVIEW_USE_WEBENGINE` is activated. Inline scripting using `<script> [...] </script>` is also possible. An example of how to add a documentation folder is available at `Examples/Plugins/ElevationFilter/`.

## Added a complex architecture plugin example

A new example that shows how to create a plugin with a complex architecture is now available.

This new example contains two plugins. One plugin uses multiple internal VTK modules. Both plugins use a third, shared, VTK module.

The example can be found in `Examples/Plugins/ComplexPluginArchitecture`.

## ParaView plugin debugging

New CMake options to debug plugin discovery and building have been added. The `ParaView_DEBUG_PLUGINS`, `ParaView_DEBUG_PLUGINS_ALL`, `ParaView_DEBUG_PLUGINS_building`, and `ParaView_DEBUG_PLUGINS_plugin` flags may be used to enable various logging for plugins.

## Plugin Location Interface

Dynamically-loaded plugins can now get the file system location of the plugin binary file (DLL, shared object) with the addition of the `pqPluginLocationInterface` class and `paraview_add_plugin_location` CMake function. This allows dynamic plugins to include text and/or data files that can be located and loaded at runtime.

## Improvements to `pqProxyWidget`

`pqProxyWidget` is the widget used to auto-generate several panels based on proxy definitions. The class supports two types of panel visibilities for individual widgets for properties on the proxy: **default** and **advanced**. These needed to match the value set for the `panel_visibility` attribute used the defining the property in ServerManager XML. `pqProxyWidget` now has API to make this configurable. One can now use arbitrary text for `panel_visibility` attribute and then select how that text is interpreted, default or advanced, by a particular `pqProxyWidget` instance. This makes it possible for proxies to define properties that are never shown in the **Properties** panel, for example, but are automatically shown in some other panel such as the **Multiblock Inspector** without requiring any custom code. For more details, see `pqProxyWidget::defaultVisibilityLabels` and `pqProxyWidget::advancedVisibilityLabels`.

## Filtering in `pqTabbedMultiViewWidget`

`pqTabbedMultiViewWidget` which is used to show multiple view layouts as tabs now supports filtering based on annotations specified on the underlying layout proxies. Checkout the `TabbedMultiViewWidgetFilteringApp.cxx` test to see how this can be used in custom applications for limiting the widget to only a subset of layouts.

## Decorator to show/hide or enable/disable properties based on session type

Certain properties on a filter may not be relevant in all types of connections supported by ParaView and hence one may want to hide (or disable) such properties except when ParaView is in support session. This is now supported using `pqSessionTypeDecorator`.

## Customizing context menus

ParaView now supports plugins that add to or replace the default context menu, via the `pqContextMenuInterface` class. The default ParaView context menu code has been moved out of `pqPipelineContextMenuBehavior` into `pqDefaultContextMenu`.

Context menu interface objects have a priority; when preparing a menu in response to user actions, the objects are sorted by descending priority. This allows plugins to place menu items relative to others (such as the default menu) as well as preempt all interfaces with lower priority by indicating (with their return value) that the behavior should stop iterating over context-menu interfaces.

There is an example in `Examples/Plugins/ContextMenu` and documentation in `Utilities/Doxygen/pages/PluginHowto.md`.

## Enable customization of scalar bar titles for array names

Settings JSON configuration files can now specify default scalar bar titles for arrays of specific names. In a settings JSON file, a `<custom title> `for `<array name>` can be specified with the following JSON structure:

```
{
  "array_lookup_tables" :
  {
    "<array name>" :
    {
      "Title" : "<custom title>"
    }
  }
}
```

Custom titles can only be read from JSON - this change does not include a mechanism to save custom titles from ParaView.

## Building on Windows with Python debug libraries supported

A new CMake option `PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE` has been added. Turn this option `ON` to build ParaView on Windows with Python debug libraries.

## Improved documentation and CMake handling of OpenGL options

The [documentation](https://kitware.github.io/paraview-docs/nightly/cxx/Offscreen.html) about OpenGL options has been improved. In addition, CMake now checks which options are compatible with each other.

## Force `QString` conversion to/from Utf8 bytes

The practice of converting `QString` data to a user's current locale has been replaced by an explicit conversion to UTF-8 encoding. This change integrates neatly with VTK's UTF-8 everywhere policy and is in line with Qt5 string handling, whereby C++ `string`s/`char*` are assumed to be UTF-8 encoded. Any legacy text files containing extended character sets should be saved as UTF-8 documents, in order to load them in the latest version of ParaView.

## Use wide string Windows API for directory listings

Loading paths and file names containing non-ASCII characters on Windows is now handled via the wide string API and uses `vtksys` for the utf8 <-> utf16 conversions. Thus the concept of converting text to the system's current locale has been completely eliminated.

## Static builds with kits enabled no longer supported

ParaView no longer allows building kits with static builds. There are issues with *in situ* modules being able to reliably load Python in such configurations. However, the configuration does not make much sense in the first place since the goal of kits is to reduce the number of libraries that need to be loaded at runtime and static builds do not have this problem.

## CGNS module migration

The following modules migrations have occurred:

  - `ParaView::cgns` → `VTK::cgns`
  - `ParaView::VTKExtensionsCGNSReader` → `VTK::IOCGNSReader`

The cache variables controlling these modules are automatically migrated,
however, the target names have not been migrated. Consumers may need updates to
use the new VTK-homed target names.
