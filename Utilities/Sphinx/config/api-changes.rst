API Changes between ParaView versions
=====================================

Changes in 5.7
--------------

Views and Layouts
-----------------

`CreateView`, `CreateRenderView`, etc. no longer automatically associates the
created view with a layout. One has to explicitly assign the view to a layout.
`simple.AssignViewToLayout` may be used to assign the view to an available
layout, or create a new one if none exists.

`detachedFromLayout` argument to all the view creation functions is now
obsolete since all views are created detached from layout.

Changes in 5.5
--------------

`SaveScreenshot` and `SaveAnimation` parameters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`ImageQuality` keyword parameter has been deprecated and will be ignored.
Instead, users are expected to provide format spefic keyword parameters that
control output quality. This allows for a fine grained control over the output
quality based on the file format, rather than hiding it under a single quality
number.

Offscreen rendering
~~~~~~~~~~~~~~~~~~~

ParaView executables now automatically choose to use offscreen rendering, if
appropriate. For Python executables, `pvbatch` automatically uses offscreen by
default (even headless i.e.  using EGL or OSMesa if built with support for the
same), and `pvpython` opts for onscreen. You can override the same by passing
command line arguments `--force-offscreen-rendering` or
`--force-onscreen-rendering`.

As a result `view.UseOffscreenRendering` property has been removed
to avoid conflicting with this automatic logic.

Likewise, `view.UseOffscreenRenderingForScreenshots` has been removed too. That
option is no longer needed as ParaView is no longer affected by overlapping
windows when capturing screenshots.

Changes in 5.1
--------------

Cube Axes no longer available
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Cube axes is no longer supported by ParaView. Hence all calls to display
properties for the same should now be simply removed from your Python code. e.g.
``display.CubeAxesVisibility = ..`` and other such properties that began with
``CubeAxes`` are no longer available and should be removed.

A complete list of display properties removed is as follows:
``CubeAxesVisibility``,
``CubeAxesColor``, ``CubeAxesCornerOffset``, ``CubeAxesFlyMode``,
``CubeAxesInertia``, ``CubeAxesTickLocation``,
``CubeAxesXAxisMinorTickVisibility``, ``CubeAxesXAxisTickVisibility``,
``CubeAxesXAxisVisibility``, ``CubeAxesXGridLines``, ``CubeAxesXTitle``,
``CubeAxesUseDefaultXTitle``, ``CubeAxesYAxisMinorTickVisibility``,
``CubeAxesYAxisTickVisibility``, ``CubeAxesYAxisVisibility``,
``CubeAxesYGridLines``, ``CubeAxesYTitle``, ``CubeAxesUseDefaultYTitle``,
``CubeAxesZAxisMinorTickVisibility``, ``CubeAxesZAxisTickVisibility``,
``CubeAxesZAxisVisibility``, ``CubeAxesZGridLines``, ``CubeAxesZTitle``,
``CubeAxesUseDefaultZTitle``, ``CubeAxesGridLineLocation``, ``DataBounds``,
``CustomBounds``, ``CustomBoundsActive``, ``OriginalBoundsRangeActive``,
``CustomRange``, ``CustomRangeActive``, ``UseAxesOrigin``, ``AxesOrigin``,
``CubeAxesXLabelFormat``, ``CubeAxesYLabelFormat``, ``CubeAxesZLabelFormat``,
``StickyAxes``, ``CenterStickyAxes``.


CameraClippingRange on render view
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
For a few releases, ``CameraClippingRange`` property would not have any effect on a render view.
Since the clipping range is now managed by the ``vtkPVRenderView`` automatically, the user is not expected
to set this property any more. This release finally removes this property. To fix any existing Python scripts,
simply remove any code ``view.CameraClippingRange = ...`` from your script.

Changes in 4.2
--------------

Changes to defaults
~~~~~~~~~~~~~~~~~~~
This version includes major overhaul of the ParaView's Python internals towards
one main goal: making the results consistent between ParaView UI and Python
applications e.g. when you create a RenderView, the defaults are setup
consistently irrespective of which interface you are using. Thus, scripts
generated from ParaView's trace which don't describe all property values may
produce different results from previous versions.


Choosing data array for scalar coloring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
In previous versions, the recommended method for selecting an array to color
with was as follows:

::

    disp = GetDisplayProperties(...)
    disp.ColorArrayName = ("POINT_DATA", "Pressure")
    # OR
    disp.ColorArrayName = ("CELL_DATA", "Pressure")

However, scripts generated from ParaView's trace, would result in the following:

::

    disp.ColorArrayName = "Pressure"
    disp.ColorAttributeType = "POINT_DATA"

The latter is no longer supported i.e. ``ColorAttributeType`` property is no
longer available. You uses ColorArrayName to specify both the array
association and the array name, similar to the API for selecting arrays on
filters. Additionally, the attribute type strings ``POINT_DATA`` and
``CELL_DATA`` while still supported are deprecated. For consistency, you use
``POINTS``, ``CELLS``, etc. instead.

::

   disp.ColorArrayName = ("POINTS", "Pressure")


Chart properties
~~~~~~~~~~~~~~~~
There are three types of changes to APIs that set chart properties.

1. Axis properties were set using arrays that contain elements for all
axes (left, bottom, right and top). Now these settings are separated
such that each axis has its own function. There are three groups of
properties affected.

Color settings used arrays of 12 elements to set the color for all
axes. In the current version we use a function for each axis, each
with 3 elements.

- ``AxisColor``
- ``AxisGridColor``
- ``AxisLabelColor``
- ``AxisTitleColor``

Font properties used arrays of 16 elements, 4 elements for each
axis. In the current version we use a function for each axis and for
each font property. See the also the section on font properties.
a. ``AxisLabelFont``
b. ``AxisTitleFont``

There are various other properties that used arrays of 4 elements, one
element for each axis.

- ``AxisLabelNotation``
- ``AxisLabelPrecision``
- ``AxisLogScale``
- ``AxisTitle``
- ``AxisUseCustomLabels``
- ``AxisUseCustomRange``
- ``ShowAxisGrid``
- ``ShowAxisLabels``

The new function names are obtained by using prefixes Left, Bottom,
Right and Top before the old function names. For example, ``AxisColor``
becomes ``LeftAxisColor``, ``BottomAxisColor``, ``RightAxisColor`` and
``TopAxisColor``.

2. Font properties were set using arrays of 4 elements. The 4 elements
were font family, font size, bold and italic. In the current version we use
a function for each font property. The functions affected are:

- ``ChartTitleFont``
- ``LeftAxisLabelFont``
- ``BottomAxisLabelFont``
- ``RightAxisLabelFont``
- ``TopAxisLabelFont``
- ``LeftAxisTitleFont``
- ``BottomAxisTitleFont``
- ``RightAxisTitleFont``
- ``TopAxisTitleFont``

The new function names can be obtained by replacing Font with FontFamily,
FontSize, Bold and Italic. So ``ChartTitleFont`` becomes
``ChartTitleFontFamily``, ``ChartTitleFontSize``, ``ChartTitleBold``,
``ChartTitleItalic``. Note that function names from bullet b to i are generated
in the previous step.

3. Range properties were set using an array of two elements. In the
current version we use individual functions for the minimum and
maximum element of the range.  Properties affected are:

- ``LeftAxisRange``
- ``BottomAxisRange``
- ``RightAxisRange``
- ``TopAxisRange``

The new function names are obtained by using Minimum and Maximum
suffixes after the old function name. So ``LeftAxisRange`` becomes
``LeftAxisRangeMinimum`` and ``LeftAxisRangeMaximum``.


Glyph filters
~~~~~~~~~~~~~

The glyph filters (``Glyph`` and ``GlyphWithCustomSource``) have been refactored
in this release. This new filters offer new APIs for sampling and masking
points. The older implementation is still available. If you want to use the
older version of the filters instead, replace the constructor functions by
``LegacyGlyph`` and ``LegacyArbitrarySourceGlyph`` respectively.

These older implementations, however, will be removed entirely in future
releases. Hence, you should consider updating the script to use the newer
version of this filter. If there is any functionality missing from the older
implementation that you find useful, please use the mailing list to report to
the developers.
