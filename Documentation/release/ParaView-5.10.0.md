ParaView 5.10.0 Release Notes
============================

# 19-node Pyramid #

ParaView now incorporates the VTK implementation of the 19-node-pyramid (vtkTriQuadraticPyramid).

More specifically, `vtkSMCoreUtilities` and `UnstructuredCellTypes` source now include `vtkTriQuadraticPyramid`.


# Add axis selection to RotationalExtrusionFilter #

Add a field to the RotationalExtrusionFilter to select the axis around which the extrusion must be done. The default stays the z-axis.


# Add favorite directories customization #

* Add "Add current directory to favorites", "Remove current directory from favorites" and "Reset favorites to system default" buttons for the favorites list in the file dialog.

* Add an "Add to favorites" option when right-clicking a directory in the files list.

* Add a "Remove from favorites" option when right-clicking a directory in the favorites list.

* Add a "Rename" option when right-clicking and item in the files list.

* Add a "Open in file explorer" option when right-clicking the files list. If the right-clicked item is a directory, it is opened in the system file explorer. If a file or nothing is select, it is the current directory that is opened.

* Add a "Delete empty directory" when right-clicking an empty directory in the files list.

* Add a "Rename label" option when right-clicking a directory in the favorites list, which can also be triggered by pressing the F2 key. It only renames the label displayed in the favorites list and not the actual folder name.

* Add standard shortcuts to some buttons in the file dialog. `Alt+Left` goes back, `Alt+Right` goes forward, `Alt+Up` goes to the parent directory and `Ctrl+N` creates a new directory.

* Add labels for favorites and recent lists to show the user what they are.

* Disable the "Create New Folder" button when opening an existing file.

* Add feedback when a favorites path does not exist : the name is greyed out and in italic, the icon is replaced by a warning and the tooltip warns that the path does not exist.


# Add hover descriptions in Edit Color Legend window #

Some widgets in the edit color legend window that did not have descriptions, now have.

Not advanced:
* Auto Orient check box
* Orientation drop down
* Title text box
* Component Title text box
* Draw annotations check box

Advanced:
* Automatic Label Format check box
* Label format text box
* Add Range Labels check box
* Range Label Format text box
* Add range annotations check box
* Draw NaN annotation check box
* NaN annotation text box


# Example Plugin for paraview_plugin_add_proxy #

A new example plugin AddPQProxy has been added
to demonstrate how to use the paraview_plugin_add_proxy
macros (add_pqproxy before ParaView 5.7.0)


# Add reader for SAVG file format from NIST #

Adds a python reader for the SAVG file format, an ascii format
supporting definition of primitives likes points, lines, and
polygons.


# Add SMP backend information to about dialog #

Adds the following in the Help->About dialog:
- SMP Tools backend in use
- SMP max number of threads


# Stride property for the animation #

It is now possible to change the granularity of the animation using the `Stride` property in the animation view.
This property only makes sense in the `Snap to Timesteps` and `Sequence` mode i.e. when there's a fixed number of frames.
The `Stride` property allows to skip a fixed number of frames to control how much frame should be displayed.
For example with a dataset having a time domain of `[0,1,2,3,4,5,6]` and when setting the stride to 2, only the times
`[0,2,4,6]` will be used.
The stride is taken into account when using the `Play`, `Go To Next Frame` or `Go To Previous Frame` buttons.
A stride of 1 will act as the default behavior i.e. no frames are skipped.


# The Threshold filter can now threshold below or above a value #

New thresholding methods have been added to the `Threshold` filter:

- `Between`: Keep values between the lower and upper thresholds.
- `Below Lower Threshold`: Keep values smaller than the lower threshold.
- `Above Upper Threshold`: Keep values larger than the upper threshold.

Previously, it was only possible to threshold between two values.

Accordingly, the property `ThresholdBetween` has been removed. Instead, the following three properties have been added: `LowerThreshold`, `UpperThreshold` and `ThresholdMethod`.


# Simulate anisotropic materials in the PBR represention #

When using the PBR representation under the lighting panel, you can now
set the anisotropy strength and anisotropy rotation for a material.
The anisotropy strength controls the amount of light reflected along the
anisotropy direction (ie. the tangent). The anisotropy rotation rotate the
tangent around the normal. Notice that the object must have normals and tangents
defined to work. You can also use a texture to hold the anisotropy strength in
the red channel, and the anisotropy rotation in the green channel.
These parameters are supported by OSPRay pathtracer.


# macOS arm64 Binaries #

ParaView is now tested on the macOS arm64 platform.

Due to this change, ParaView binaries are now available using the processor
name according to the platform in the binary filename rather than a generic
"32Bit" or "64Bit" indicator.

Linux: `x86_64`
macOS: `arm64` and `x86_64`
Windows: `AMD64`


Render View Background Color
----------------------------

Render View (and related views) now have a property
**UseColorPaletteForBackground** which indicates if the view-specific background
overrides should be used or the view should simply use the global color palette
to determine how the background is rendered. Previously, it was confusing to
know which views were using the global color palette and which ones were using
user overridden values for background color.

Also, instead of having separate boolean properties that are largely mutually
exclusive, the view now adds a `BackgroundColorMode` property that lets the user
choose how the background should be rendered.
Supported values are `"Single Color"`, `"Gradient"`, `"Texture"` and `"Skybox"`.

This change impacts the Python API. `UseGradientBackground`, `UseTexturedBackground`
and `UseSkyboxBackground` properties have been replaced by `BackgroundColorMode`
which is now an enumeration which lets user choose how to render the background.
Also, unless compatibility <= 5.9 was specified, view specific background color
changes will get ignored unless `UseColorPaletteForBackground` property is not
set to `0` explicitly.


# better-fortran-catalyst-bindings #

Fortran bindings to Catalyst APIs has been improved. There is now a `catalyst`
and `catalyst_python` module available with namespaced APIs.

# `catalyst` module #

  - `coprocessorinitialize()` is now `catalyst_initialize()`
  - `coprocessorfinalize()` is now `catalyst_finalize()`
  - `requestdatadescription(step, time, need)` is now `need = catalyst_request_data_description(step, time)`
  - `needtocreategrid(need)` is now `need = catalyst_need_to_create_grid()`
  - `coprocess()` is now `catalyst_process()`

The `need` return values are of type `logical` rather than an integer.

## `catalyst_python` module

  - `coprocessorinitializewithpython(filename, len)` is now `catalyst_initialize_with_python(filename)`
  - `coprocessoraddpythonscript(filename, len)` is now `catalyst_add_python_script(filename)`


# Catalyst 2 Steering #

Add steering capabilities to Catalyst 2, by implementing the `catalyst_results` method in ParaView Catalyst.

With steering, the users are now able to change simulation parameters through the ParaView Catalyst GUI. See `CxxSteeringExample` in `Examples/Catalyst2` folder for live example of the feature.


# Add Conduit Node IO and Catalyst Replay Executable ## #

To assist in debugging in-situ pipelines, Catalyst now
supports `conduit_node` I/O. The `params` argument to each invocation of
`catalyst_initialize`, `catalyst_execute`, and `catalyst_finalize` can be
written to disk. These can later be read back in using the `catalyst_replay`
executable. See the corresponding documentation:
https://catalyst-in-situ.readthedocs.io/en/latest/catalyst_replay.html


Catalyst now has a TimeValue option for triggering output.
A new TimeValue trigger option for triggering output through Catalyst has been added
which is based on the amount of simulation time that has passed since the last
output of the specified extract. This trigger option is more appropriate for
simulation codes that have variable timestep lengths.


# Support for Polyhedral elements in Catalyst V2 #

Catalyst Adaptor API V2 now supports polyhedral elements.
This is done by adding support for polyhedral elements
in `vtkConduitSource`. A new example, `CxxPolyhedra`, demonstrates
how to use Conduit Mesh Blueprint to communicate information about
polyhedral elements.


# Change Color Map Editor behavior #

* Add a combo box to quick apply a preset from the `Default` group directly from the Color Map Editor.

* Add group selection for custom presets. Add a `Groups` field in the imported `json` presets files to add the preset to the specified groups. For example this preset will be added to the `Default`, `CustomGroup1` and `Linear`.
  ```json
  [
    {
      "ColorSpace" : "RGB",
      "Groups" : ["Default", "CustomGroup1", "Linear"],
      "Name" : "X_Ray_Copy_1",
      "NanColor" :
      [
        1,
        0,
        0
      ],
      "RGBPoints" :
      [
        0,
        1,
        1,
        1,
        1,
        0,
        0,
        0
      ]
    }
  ]
  ```
  In this case, `CustomGroup1` does not exist beforehand so it will be created on import. If the `Groups` field does not exist, the preset is added to the `Default` and `User` groups.

* The `DefaultMap` field is no longer used as it is redundant with the new `Groups` field.

* Any preset can now be added or removed from the `Default` group.

* Make the presets dialog non-modal

* Imported presets are displayed in italics to be able to differentiate them from base presets.


# Simulate clear coat layer in the PBR represention #

When using the PBR representation under the lighting panel, you can now
add a second layer on top of the base layer. This is useful to simulate
a coating on top of a material. This layer is dielectric (as opposed with
metallic), and can be configured with various parameters. You can set the
coat strength to control the presence of the coat layer (1.0 means
strongest coating), the coat roughness and the coat color. You can
also choose the index of refraction of the coat layer as well as the
base layer. The more the index of refraction goes up, the more specular
reflections there are.
You can also use a texture to normal map the coating.
These parameters are supported by OSPRay pathtracer.


# Command line option parsing #

The command line options parsing code has been completely refactored. ParaView
now uses [CLI11](https://github.com/CLIUtils/CLI11). This has implications for
users and developers alike.

# Changes for users #

There way to specify command line options is now more flexible. These can be
provided as follows:

* `-a` : a single flag / boolean option
* `-f filename`: option with value, separated by space
* `--long`:  long flag / boolean option
* `--file filename`: long option with value separated using a space
* `--file=filename`: long option with value separated by equals

Note this is a subset of ways supported by CLI11 itself. This is because
ParaView traditionally supported options of form `-long=value` i.e. `-` could be
used as the prefix for long-named options. This is non-standard and now
deprecated. Instead, one should add use `--` as the prefix for such options e.g.
`-url=...` becomes `--url=...`. Currently, this is done automatically to avoid
disruption and a warning is raised. Since this conflicts with some of the other
more flexible ways of specifying options in `CLI11`, we limit ourselves to the
ways listed above until this legacy behaviour is no longer supported.

The `--help` output for all ParaView executables is now better formatted.
Options are grouped, making it easier to inspect related options together.
Mutually exclusive options, deprecated options are clearly marked to minimize
confusion. Also, in most terminals, the text width is automatically adjusted to
the terminal width and text is wrapped to make it easier to read.

Several options supports overriding default values using environment variables.
If the option is not specified on the command line, then that denoted
environment variable will be used to fetch the value for that option (or flag).

## Changes for developers

`vtkPVOptions` and subclasses are deprecated. Instead of a single class that
handled the parsing of the defining of command line flags/options, command line arguments,
and then keep state for the flag/option selections, the new design uses two
different classes. `vtkCLIOptions` handle the parsing (using CLI11), while
singletons such as `vtkProcessModuleConfiguration`, `vtkRemotingCoreConfiguration`, `pqCoreConfiguration`
maintain the state and also populate `vtkCLIOptions`  with supported flags/options. Custom applications
and easily add their own `*Configuration` classes to populate `vtkCLIOptions` to
custom options of override the default ParaView ones. If your custom code was
simply checking user selections from `vtkPVOptions` or subclasses, change it to
using the corresponding `*Configuration` singleton.


# Added a complex architecture plugin example #

A new example that show how to create complex architecture
in a plugin is now available.

This new example contains two plugins, one of them using multiple
internal VTK module and both of them using a third, shared, VTK module.

It can be found in Examples/Plugins/ComplexPluginArchitecture.


ParaView now supports plugins that add to or replace the default context menu,
via the pqContextMenuInterface class. The default ParaView context menu code
has been moved out of pqPipelineContextMenuBehavior into pqDefaultContextMenu.

Context menu interface objects have a priority; when preparing a menu in
response to user action, the objects are sorted by descending priority.
This allows plugins to place menu items relative to others (such as the
default menu) as well as preempt all interfaces with lower priority by
indicating (with their return value) that the behavior should stop iterating
over context-menu interfaces.

There is an example in `Examples/Plugins/ContextMenu` and documentation
in `Utilities/Doxygen/pages/PluginHowto.md`.


# Added a ConvertToPointCloud filter #

A new filter, ConvertToPointCloud, has been added.
It let users convert any dataset into a vtkPolyData
containing the same points as the inputs but with either
a single poly vertex cell, many vertex cells or no cells.


# Enable customization of scalar bar titles for array names #

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


# Changes to vtkPVDataInformation #

vtkPVDataInformation implementation has been greatly simplified.

vtkPVDataInformation no longer builds a complete composite data hierarchy
information. Thus, vtkPVCompositeDataInformation is no longer populated
and hence removed. This simplifies the logic in vtkPVDataInformation
considerably.

vtkPVDataInformation now provides assess to vtkDataAssembly
representing the hierarchy for composite datasets. This can be used by
application to support cases where information about the composite
data hierarchy is needed. For vtkPartitionedDataSetCollection, which can
other assemblies associated with it, vtkPVDataInformation also
collects those.

For composite datasets, vtkPVDataInformation now gathers information
about all unique leaf datatypes. This is useful for applications to
determine exactly what type of datasets a composite dataset is comprised
of.

vtkPVTemporalDataInformation is now simply a subclass of
vtkPVDataInformation. This is possible since vtkPVDataInformation no
longer includes vtkPVCompositeDataInformation.


# New default view setting 'Empty' #

Under the "Edit.. Settings... Default View Settings", you can now select the "Empty" default value.
This setting will open ParaView with an empty view that lets you select the available views.


Deprecate RealTime animation mode

The 'RealTime' animation mode is deprecated and will be removed in a future release.
This is necessary because:
 - ParaView does not aim do be a video editor.
 - ParaView processes data and takes the time it needs so real time is really unlikely to happen as expected.

see also https://discourse.paraview.org/t/paraview-time-refactoring/5506/6


# Change the "Community Support" link from paraview.org to discourse.paraview.org #

This link will send you to the ParaView Support discourse page to open a new topic or search for an existing one.


# Added support for empty Mesh Blueprint coming from Conduit to Catalyst #

Catalyst 2.0 used to fail when the simulation code sends a mesh through
Conduit with a full Conduit tree that matches the Mesh Blueprint but
without any cells or points. This can for instance happen on
distributed data. This commit checks the number of cells in the Conduit
tree and returns an empty vtkUnstructuredGrid when needed. This commit
only fixes the issue for unstructured meshes. This commit also fixes a
crash on meshes containing a single cell (rare occasion, but can
happen).

See https://gitlab.kitware.com/paraview/paraview/-/issues/20833


# Changes to Extract Block #

**Extract Block** filter now supports choosing blocks to extract using selector
expressions. This makes the block selection  more robust to changes to the input
hierarchy.


# # Fetching data to the client in Python #

`paraview.simple` now provides a new way to fetch all data from the
data server to  the client from a particular data producer. Using
`paraview.simple.FetchData` users can fetch data from the data-server locally
for custom processing.

Unlike `Fetch`, this new function does not bother applying any transformations
to the data and hence provides a convenient way to simply access remote data.

Since this can cause large datasets to be delivered the client, this must be
used with caution in HPC environments.


# Filtering in pqTabbedMultiViewWidget #

`pqTabbedMultiViewWidget` which is used to show multiple view layouts as tabs
now supports filtering based on annotations specified on the underlying layout
proxies. Checkout the `TabbedMultiViewWidgetFilteringApp.cxx` test to see how this
can be used in custom applications for limiting the widget to only a
subset of layouts.


# Find Data Panel #

**Find Data** dialog has been replaced by **Find Data** panel which offers
similar capabilities except as a dockable panel.

**Selection Display Inspector** been removed since **Find Data** panel presents
the same information under the **Selection Display** section.


# Grid Connnecivity has been removed #

Filter named **Grid Connectivity** has been removed. The **Connectivity** filter
should support a wider set of use-cases than the **Grid Connectivity** filter and
should be preferred.


# Improvements to Group Datasets filter #

**Group Datasets** filter now supports multiple types of
outputs including `vtkPartitionedDataSetCollection`, `vtkPartitionedDataSet`,
and `vtkMultiBlockDataSet`. One can also assign names to inputs which then get
assigned as blocked names.

![group-datasets](img/5.10.0/group-datasets.png)


# Histogram filter improvements #

The Histogram filter has been enhanced in several ways:

* Values from hidden points or cells are no longer included in the histogram results.
* The data range used for automatic bin range values now excludes hidden points and cells.
* A new option, **Normalize**, makes the filter produce normalized histogram values for applications where knowing the fraction of values that fall within each bin is desired.


# HPC benchmarks and validation suite #

To make it easier to test and validate HPC builds, we have added a new package
under the `paraview` Python package called `tests`. This package includes
several modules that test and validate different aspects of the ParaView build.

The tests can be run as follows:


    # all tests
    pvpython -m paraview.tests -o /tmp/resultsdir

    # specific tests
    pvpython -m paraview.tests.verify_eyedomelighting -o /tmp/eyedome.png
    pvpython -m paraview.tests.basic_rendering -o /tmp/basic.png


Use the `--help` or `-h` command line argument for either the `paraview.tests`
package or individual test module to get list of additional options available.

The list is expected to grow over multiple releases. Suggestions to expand this validation
test suite are welcome.


# Improve documentation mecanism for ParaView plugins #

Add 3 new CMake options to the `paraview_add_plugin` function :

 - **DOCUMENTATION_ADD_PATTERNS**: If specified, add patterns for the documentation files within `DOCUMENTATION_DIR` other than the default ones (i.e. `*.html`, `*.css`, `*.png`, `*.js` and `*.jpg`). This can be used to add new patterns (ex: `*.txt`) or even subdirectories (ex: `subDir/*.*`). Subdirectory hierarchy is kept so if you store all of your images in a `img/` sub directory and if your html file is at the root level of your documentation directory, then you should reference them using `<img src="img/my_image.png"/>` in the html file.
 - **DOCUMENTATION_TOC**: If specified, the function will use the given string to describe the table of content for the documentation. A TOC is diveded into sections. Every section point to a specific file (`ref` keyword) that is accessed when double-clicked in the UI. A section that contains other sections can be folded into the UI. An example of such a string is :
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
 - **DOCUMENTATION_DEPENDENCIES**: Targets that are needed to be built before actually building the documentation. This can be useful when the plugin developer relies on a third party documentation generator like Doxygen for example.

See `Examples/Plugins/ElevationFilter` for an example of how to use these features.


# Information Panel #

The **Information Panel** has been redesigned to improve the way information is
presented to the users. Besides general layout improvements that make the panel
more compact and consistent with other panels, the changes are as follows:

* When showing composite datasets, one can view information specific to a subtree by
  selecting that subtree in the panel. However, if the data changes, for
  example, due to animation, the selection was lost making it tedious to monitor
  data information changes for a specific subtree. This has been fixed.

* All numbers now use locale specific formatting. For example, for US-EN based
  locale, large number are formatting by placing commas. Memory used is shown in
  KB, MB, or GB as appropriate.

>![information-panel](img/5.10.0/information-panel.png)


# IOSS reader for Exodus files #

ParaView now use IOSS library to read Exodus files. This new reader,
`vtkIOSSReader`, was introduced in 5.9 as a plugin. In this release,
ParaView now uses this new reader as the default reader for all Exodus files.
The previous reader is still available and can be used in the UI by simply
loading the **LegacyExodusReader** plugin. XML state files and Python scripts
using the old Exodus reader explicitly will continue to work without any
changes.

The IOSS reader has several advantages over the previous implementation. One of
the major ones is that it uses the modern `vtkPartitionedDataSetCollection` as
the output data-type instead of `vtkMultiBlockDataSet`.

# IOSS reader for CGNS files #

In additional to Exodus, IOSS reader now supports reading CGNS files as well.
Note, the reader only supports a subset of CGNS files that are generated using
the IOSS library and hence may not work for all CGNS files. The CGNS reader is
still the preferred way for reading all CGNS files.


# Javascript support for documentation in Paraview #

It is possible to add a `.js` file to a plugins documentation and reference it into an `.html` file.
However JS scripts will only be evaluated if `PARAVIEW_USE_WEBENGINE` is activated.
Inline scripting using `<script> [...] </script>` is also possible.
An example of how to add a documentation folder is available at `Examples/Plugins/ElevationFilter/`.

There was a bug in paraview where even if the QtWebEngine cmake option was activated the backend for displaying
help in paraview was not QtWebEngine. Reason of this bug is that the CMake option was never defined in the C++ code.
This was fixed in MR paraview/!4669 (January 2020).


# Improvements to 'LoadState' Python API #

When loading pvsm state files in Python using `LoadState` function, it was
tricky to provide arguments to override data files used in the state file.
The `LoadState` function has now been modified to enable users to specify
filenames to override using a Python dictionary. The Python trace captures
this new way of loading state files. Invocations of this function using
previously used arguments is still supported and will continue to work.

Some examples:

    # use data files under a custom directory #
    LoadState(".....pvsm",
              data_directory="/...",
              restrict_to_data_directory=True)

    # explicitly override files #
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


# Added icons in macOS 11 style ## #

The icons in macOS 11 Big Sur are in a new unified style. The icon files named
`Icon-MacOS-*.png` are the new files going into `pvIcon.icns` so they are
separate from Windows and Linux icons.


# The filters Gradient and Gradient Of Unstructured DataSet have been merged #

Before ParaView 5.10, these two filters could be used to compute the gradient of a dataset:
- `Gradient` based on `vtkImageGradient` for `vtkImageData` only
- `Gradient Of Unstructured DataSet` based on `vtkGradientFilter` for all `vtkDataSet`

These filters have been replaced with a unique `Gradient` filter based on `vtkGradientFilter` and including the functionalities from the former `Gradient` filter. The default behavior always uses the `vtkGradientFilter` implementation.

For `vtkImageData` objects, it is still possible to use the `vtkImageGradient` implementation through the `BoundaryMethod` property, which has two options defining the gradient computation at the boundaries:
- `Smoothed` corresponds to the `vtkImageGradient` implementation (the old `Gradient`) and uses central differencing for the boundary elements by duplicating their values
- `Non-Smoothed` corresponds to the `vtkGradientFilter` implementation (the old `Gradient Of Unstructured DataSet`) and simply uses forward/backward differencing for the boundary elements

For all other `vtkDataSet` objects, the filter usage is unchanged.


# Add MergeTime filter #

MergeTime filter takes multiple temporal datasets as input and synchronize them.

The output data is a multiblock dataset containing one block per input dataset.
The output timesteps is the union (or the intersection) of each input timestep lists.
Duplicates time values are removed, dependending on a tolerance, either absolute or relative.


# Merge Vector Components #

The goal of this merge request is to expose vtkMergeVectorComponents that has been merged into vtk.
See: https://gitlab.kitware.com/vtk/vtk/-/merge_requests/8135

For the purpose of this MR:
* A XML that exposes vtkMergeVectorComponents to Paraview has been designed
* A test that uses the vtkMergeVectorComponents filter has been developed

Both of these Merge requests will address the issue https://gitlab.kitware.com/paraview/paraview/-/issues/20491


# H.264-encoded MP4 video export on Windows #

ParaView Windows binaries now offer export of H.264-encoded MP4 files.
The frame rate and bit rate are both exposed as options.


# Multicolumn and multiline support in equation rendering #

The rendering of equations using MathText (with matplotlib enabled via the cmake option VTK_MODULE_ENABLE_VTK_RenderingMatplotlib) now supports multiline and multicolumn. You can define a new line by writing a new line in the text source, and you can define a new column by entering the character '|'. You can still write a '|' by escaping it with a backslash ('\|')


# Node editor plugin #

It is now possible to visualize the pipeline as a node graph like other popular software like Blender.
This is embedded in the NodeEditor plugin and can be compiled by activating the `PARAVIEW_PLUGIN_ENABLE_NodeEditor` cmake option when compiling ParaView.

The users control are :

  * Filters/Views are selected by double-clicking their corresponding node labels (hold CTRL to select multiple filters).
  * Output ports are selected by double-clicking their corresponding port labels (hold CTRL to select multiple output ports).
  * Nodes are collapsed/expanded by right-clicking node labels.
  * Selected output ports are set as the input of another filter by double-clicking the corresponding input port label.
  * To remove all input connections CTRL+double-click on an input port.
  * To toggle the visibility of an output port in the current active view SHIFT+left-click the corresponding output port (CTRL+SHIFT+left-click shows the output port exclusively)


# no-static-kits #

ParaView no longer allows building kits with static builds. There are issues
with InSitu modules being able to reliably load Python in such configurations.
However, the configuration doesn't make much sense in the first place since the
goal of kits is to reduce the number of libraries that need to be loaded at
runtime and static builds do not have this problem.


# Add OMF Reader #

ParaView now has a reader for [Open Mining Format](https://omf.readthedocs.io/en/stable/index.html) files.
The users can specify which elements (i.e., point set, line set, surface, or volume) should be loaded.


# Improvements of the documentation and cmake management about OpenGL options ## #

The documentation about OpenGL options (https://kitware.github.io/paraview-docs/nightly/cxx/Offscreen.html)
has been improved as well as cmake checks of which options are compatible with each other.

`PARAVIEW_USE_QT` should not be used to detect if the `paraview` Qt executable
has been built, instead, `TARGET ParaView::paraview` should be used.


# Add OpenVDB Writer #

ParaView now has a writer for [OpenVDB Format](https://www.openvdb.org/) files.
The writer currently works with vtkDataSets and time-series data.


# OpenVR Plugin improvements #

Many updates to improve the OpenVR plugin support in Paraview

- Added a "Come to Me" button to bring other collaborators to your current location/scale/pose

- Fixed crop plane sync issues and a hang when in collaboration

- Support desktop users in collaboration with an "Attach to View" option that makes the current view behave more like a VR view (shows avatars/crop planes etc)

- Added a "Show VR View" option to show the VR view of the user when they are in VR. This is like the steamVR option but is camera stabilized making it a better option for recording and sharing via video conferences.

- Broke parts of vtkPVOpenVRHelper into new classes named vtkPVOpenVRExporter and vtkPVOpenVRWidgets. The goal being break what was a large class into smaller classes and files to make the code a bit cleaner and more compartmentalized.

- Add Imago Image Support - added support for displaying images strings are found in the dataset's cell data (optional, off by default)

- Fix thick crop stepping with in collaboration

- Update to new SteamVR Input API, now users can customize their controller mapperings for PV. Provide default bindings for vive controller, hp motion controller, and oculus touch controller

- Add an option controlling base station visibility

- Reenabled the option and GUI settings to set cell data value in VR

- Adding points to a polypoint, spline, or polyline source when in collaboration now works even if the other collaborators do not have the same representation set as active.


# OSPRay Material Editor #

A new OSPRay material editor widget has been added to control the OSPRay materials by enabling it under the "View... Material Editor" menu. You need to compile ParaView with `PARAVIEW_ENABLE_RAYTRACING` cmake option set to ON to enable the editor widget.

This editor allows to :

* Load a material library using a .json file or create materials directly in the GUI
* Edit / Add / Remove properties on the materials
* Attach the selected material to the current selected source


# vtkExtractHistogram is now multithreaded #

ParaView's filter `vtkExtractHistogram` has been multithreaded using `vtkSMPTools`.

Additionally, `TestExtractHistogram` test has been re-enabled.


# ParaView Plugin debugging #

New CMake options to debug plugin discovery and building has been added. The
`ParaView_DEBUG_PLUGINS`, `ParaView_DEBUG_PLUGINS_ALL`,
`ParaView_DEBUG_PLUGINS_building`, and `ParaView_DEBUG_PLUGINS_plugin` flags
may be used to enable various logging for plugins.


# ParaViewWeb #

ParaViewWeb now supports [unsigned] long long data arrays when retrieving
data information.


# Plugin Location Interface #

Dynamically-loaded plugins can now get the file system location of the
plugin binary file (DLL, shared object) with the addition of the
`pqPluginLocationInterface` class and `paraview_add_plugin_location` cmake
function. This allows dynamic plugins to include text and/or data files
that can be located and loaded at runtime.


The goal of this merge is to add support for polygonal cells in the vtkConduitSource to allow for passing of cells of shape polygonal from a Conduit tree to VTK.

see https://gitlab.kitware.com/paraview/paraview/-/merge_requests/5047
and https://gitlab.kitware.com/paraview/paraview/-/issues/20828


# ProxyListDomain support default for group #

Using the following syntax

```
    <ProxyListDomain name="proxy_list">
      <Group name="sources" default="SphereSource" />
    </ProxyListDomain>
```

A proxy list domain containing a group can define the default proxy to use

Most filter using ProxyListDomain have been converted to use group and default
in order for it to be easier to add new proxy to a proxy list domain trough plugins.

A example plugin demonstrating this, AddToProxyGroupXMLOnly.


# Improvements to pqProxyWidget #

`pqProxyWidget` is the widget used to auto-generate several panels based on
proxy definitions. The class supports two types of panel visibilities for
individual widgets for properties on the proxy: **default** and **advanced**.
These needed to match the value set for the `panel_visibility` attribute used
the defining the property in ServerManager XML. `pqProxyWidget` now has API to
make this configurable. One can now use arbitrary text for `panel_visibility`
attribute and then select how that text is interpreted, default or advanced, by
a particular `pqProxyWidget` instance. This makes it possible for proxies to
define properties that are never shown in the **Properties** panel, for example,
but are automatically shown in some other panel such as the **Multiblock
Inspector** without requiring any custom code. For more details, see
`pqProxyWidget::defaultVisibilityLabels` and
`pqProxyWidget::advancedVisibilityLabels`.


# PVSC updates #

ParaView server configuration files (PVSC) now support two new predefined
variables:

1. `PV_APPLICATION_DIR`: This is set to the directory containing the application
   executable. For macOS app bundles, for example, this will be inside the
   bundle. This is simply `QCoreApplication::applicationDirPath`.
2. `PV_APPLICATION_NAME`: This is set to the name of the Qt application as
   specified during application initialization. This is same as
   `QCoreApplication::applicationName`.


# Python initialization during import #

To make creation of various proxies easier, ParaView defines classes for each
known proxy type. These class types were immediately defined when `paraview.simple`
was imported or a connection was initialized. Creation of these class types is
now deferred until they are needed. This helps speed up ParaView Python
initialization.

This change should be largely transparent to users except for those who were
directly accessing proxy types from the `paraview.servermanager` as follows:

    # will no longer work
    cls = servermanager.sources.__dict__[name]

    # replace as follows (works in previous versions too)
    cls = getattr(servermanager.sources, name)


# Building on Windows with Python debug libraries supported #

A new CMake option `PARAVIEW_WINDOWS_PYTHON_DEBUGGABLE` has been added.
Turn this option `ON` to build ParaView on Windows with Python debug libraries.


The python editor has been improved with new features:
- faster syntax highlighting
- undo/redo capabilities
- more robust file saving


# Python Editor improvements #

The following was added to the paraview python script editor:

- auto save of opened buffers in swap files and possibility of recovery in case paraview crashes
- opening of multiple tabs in the editor
- integration with the paraview trace functionality in its own tab
- improvements of the undo/redo feature in the editor
- a new entry in the paraview edit menu to access the editor directly


# Extractor for recolorable images #

This release reintroduces the experimental recolorable images generation
capability. It is exposed using **Extractors**. A new experimental extractor
called **Recolorable Image** is now available. The extractor is experimental and
requires specific conditions to work.


# Resample With Dataset can work with partial arrays #

Previously, **Resample With Dataset** would not pass partial arrays from composite dataset input. A new option, **PassPartialArrays** has been added. When on, partial arrays are sampled from the source data.

For all those blocks where a particular data array is missing, this filter uses `vtkMath::Nan()` for `double` and `float` arrays, and 0 for all other types of arrays (e.g., `int`, `char`, etc.)


# Reverse the modifier key for screenshots #

When clicking the screenshot capture button, a simple click now saves the screenshot to a file and a click with a modifier saves it to the clipboard, which is the opposite of what it was before, including the ParaView 5.9 release.

When saving a screenshot to the clipboard, the view now blinks.


# Rotate the skybox #

You can now rotate the Skybox horizontally with Control + Left Click.
Notice that this binding was previously set to rotate the view. You can still rotate the view with Left Click.
This is useful when using PBR shading and Image Based Lighting and you want to change the reflection
without moving the geometry.


# Short descriptive title #

This is a sample release note for the change in a topic.
Developers should add similar notes for each topic branch
making a noteworthy change.  Each document should be named
and titled to match the topic name to avoid merge conflicts.

Tips for writing a good release note:

* Prefer active voice:

  ** Bad: A function to X was added to ParaView.

  ** Good: ParaView now provides function X.

* Write to the user, not about the user.

  ** Bad: Users can now do X with Y.

  ** Good: You can now do X with filter Y.

* Avoid referring to the current commit or merge request

* Add images to Documentation/release/dev


The SEP reader can now read for up to `32` dimensions and output either 2D or 3D scalar fields.
Be careful though as the proxy settings have changed: the default parameters for the scalar
field may not necessarily corresponds to the original one, leading to different output.


# Decorator to show/hide or enable/disable properties based on session type #

Certain properties on a filter may not be relevant in all types of connections
supported by ParaView and hence one may want to hide (or disable) such
properties except when ParaView is in support session. This is now supported
using `pqSessionTypeDecorator`.


# String substitutions using {fmt} #

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

``PythonCalulator`` used to use ``time_value``, and ``time_index`` in the provided expression. Now it can also use ``{timevalue} == {CALCULATOR_timevalue}``, and ``{timeindex} == {CALCULATOR_timeindex}``.


# TextStyleCustomization #

The text source has been updated and now lets you customize
the borders and the background of the text. For the borders,
it is now possible to draw round corners, choose the color as
well as the thickness. A combo box allows you to define the visibility
of the border (only on hover, always or never).
For the text, you can now choose the background color and opacity
and the right and top padding between the border and the text.


# Update vtkPVArrayCalculator #

The goal of this MR is to address the known issues https://gitlab.kitware.com/paraview/paraview/-/issues/20030 and https://gitlab.kitware.com/paraview/paraview/-/issues/20602.

Those fixes were possible thanks to the MR https://gitlab.kitware.com/vtk/vtk/-/merge_requests/8182/ which addressed what happens when a variable name is not sanitized from the side of the ``vtkArrayCalculator``.
``vtkPVArrayCalculator`` ensures now that the arrays that can be used must have a valid variable name.
``pqCalculatorWidget`` ensures now that the arrays that the user can select from the drop-down menus can actually be used.

A variable name is considered valid if it's sanitized or enclosed in quotes:
* If the array name is named `Array_B` (**therefore sanitized**) then the added variables that can be used in the expression are `Array_B` & `"Array_B"`
* If the array name is named `"Test_1"` (**therefore not sanitized but enclosed in quotes**) then the added variables that can be used in the expression are `"Test_1"`
* If the array name is named `Pressure (dyn/cm^2^)` or `1_test` (**therefore neither sanitized nor enclosed in quotes**) then the added variables that can be used in the expression are `"Pressure (dyn/cm^2^)"` or `"1_test"` respectively.


# Force QString conversion to/from Utf8 bytes #

The practice of converting QString data to a user's current locale has been replaced by an explicit conversion to Utf8 encoding. This change integrates neatly with VTK's utf8 everywhere policy and is in line with Qt5 string handling, whereby C++ strings/char* are assumed to be utf8 encoded. Any legacy text files containing extended character sets should be saved as utf8 documents, in order to load them in the latest version of Paraview.

## Use wide string Windows API for directory listings

Loading paths and file names containing non-ASCII characters on Windows is now handled via the wide string API and uses vtksys for the utf8 <-> utf16 conversions. Thus the concept of converting text to the system's current locale has been completely eliminated.


# Add spaces in window location strings #

Window location names, such as `LowerLeftCorner`, now have spaces in between, such as `Lower Left Corner`, to be more human friendly.


# View Property WindowResizeNonInteractiveRenderDelay #

A new double property, 'WindowResizeNonInteractiveRenderDelay', on the view has been added
that lets you set the time in seconds before doing a full-resolution render after the last WindowResize
event. This allows the view to render interactively when resizing a window, and can improve the
performance when using large data.


# New buttons ResetCameraClosest and ZoomClosestToData #

Two new buttons 'ResetCameraClosest' and 'ZoomClosestToData' has been added to the right of the existing "Reset Camera" and "Zoom To Data" buttons.
These buttons reset the camera to maximize the occupation, in the screen, of the whole scene bounding box (ResetCameraClosest)
or of the active source bounding box (ZoomClosestToData).


# zSpace plugin #

A new Microsoft Windows only plugin named 'zSpace' has been added. It adds
a new view named 'zSpaceView' that let a user interact with a zSpace
device directly. This device is designed to work with Crystal Eyes
stereo, in full screen or in a cave display. The zSpace glasses
use a head tracking system that allow the user to look around 3D
object by moving his head.

This plugin requires a [zSpace System Software >= 4.4.2](https://support.zspace.com/s/article/zSpace-System-Software-release-Required?language=en_US),
and a SDK version >= 4.0.0.
It was tested on a zSpace 200 device but should be compatible with more recent devices as well.
