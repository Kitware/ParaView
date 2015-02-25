# ParaViewWeb Visualizer

## Introduction

ParaView comes with several example web applications, the most complete and
full-featured of which is the Web Visualizer.  This application provides
roughly 85% of the features that can be found in the ParaView desktop
application.  The purpose of this guide is to familiarize users with the Web
Visualizer and provided detailed instructions on its use that cannot be found
elsewhere.

{@img images/pvweb-app-smaller.png The ParaViewWeb Visualizer application }

### Quick Start Guide

The following link points to a video which illustrates many of the features of
the ParaViewWeb Visualizer.

[![ParaViewWeb Visualizer - Quick Start](https://i.vimeocdn.com/video/503531370_150x84.jpg)](https://vimeo.com/116987128)

## Overview of the interface

This section gives a brief overview of the main components of the ui.

### Main toolbar

Lets first take a look at the toolbar, which is always visible at the top of
the application.  The buttons on the toolbar are identified in the image below.

{@img images/pvweb-toolbar-sm.png Visualizer toolbar buttons}

### Inspector Panel

Next we'll take a look at the inspector panel, which is on the left side of
the application, and which can be hidden or shown by clicking on the "W" icon
on the far left side of the toolbar.

{@img images/toggle-inspector-trans.png Toggle visibility of the inspector panel }

#### Pipeline editor

Depending on which of the other buttons in the toolbar is clicked, the
contents of the inspector can vary.  To see the pipeline editor (which appears
together with the proxy property editor), you must click the toolbar button to
show the pipeline.

{@img images/show-pipeline-trans.png Show the pipeline and proxy editors in the inspector panel }

The pipeline editor is shown below.  Pipeline components associated with an
unfilled circle (e.g. "StreamTracer1" in the image below) have been hidden and
are not shown in the 3D render view.  Components associated with a filled
circle (e.g. "Tube1" and "Clip1" in the image below) are visible in the 3D
render view.  The currently selected component is active, and it is associated
with a circle which is outlined in black and either filled or unfilled
depending on its visibility.  In the imae below, "Contour1" is the active
pipeline component, and it is also currently visible.  The properties
displayed in the proxy property editor always correspond to the currently
selected (active) pipeline component.

{@img images/pvweb-pipeline.png Pipeline editor }

#### Proxy property editor

The proxy editor is shown below.  It contains four collapsable sections of
properties that can be expanded to show properties or collapsed to hide them.  The
four sections are labelled "Color Management", "Source", "Representation", and
"View".  Clicking on the "Toggle Advanced Properties" shows or hides properties
marked in ParaView as advanced, and any of the above-mentioned sections may or
may not have advanced properties.  All but the most commonly used properties
are marked as advanced, so by default only the most commonly used properties
are shown.  The properties of any filter or source can be edited by first
selecting the filter or source of interest, then changing property values in
the proxy property editory, and finally clicking the apply button.

{@img images/pvweb-proxy-editor-sm.png Proxy property editor }

## Adding sources/filters and updating properties

To add a source, click the toolbar button to show sources/filters.

{@img images/show-sources-trans.png Show available sources and filters in the inspector panel }

If the pipeline is currently empty when you click this button, or if you do
not have any pipeline components already selected, then only sources will be
shown.  This is because filters are always added to the currently selected
element in the pipeline.  If you do have something selected in the pipeline,
then you should see both sources and filters, as in the following image.

{@img images/sources-filters.png Available sources and filters }

As you can see in the above image, sources are shown with a database icon to
the left, while filters are associated with a plus icon.  Clicking on a source
adds the source to the pipeline, clicking on a filter adds the filter to the
pipeline and attaches it to the whichever pipeline component is currently
selected.

As soon as you have clicked on a source or filter, and it has been added to
the pipeline, the inspector panel changes to show the pipeline and proxy
editor with the newly added source or filter selected.  Now you can change
property values on the new source or filter by first editing all color,
source, representation, or view properties you want to change, and then
clicking the apply button when you are finished.

The following link points to a short video illustrates the steps described above.

[![Adding source/filters](https://i.vimeocdn.com/video/505841297_150x84.jpg)](https://vimeo.com/118731690)

## Loading data

Click the toolbar button shown below.

{@img images/show-filelist-trans.png Click to show the list of files on the server }

Now you can interact with the file browser, refer to the image below for a
detailed view.  Click on an entry with a folder icon next to it to go into
that directory.  As you navigate through the file system, the white bar above
the list of files and directories is updated to show your current path.  This
bar is interactive so that you can get back to the root or any previous level
by clicking on a directory in the bar.  Once you find the directory of
interest, you can load a file by clicking on it.  You can load any type of
file that paraview can load, including all VTK data types and ParaView state
files.

{@img images/file-browser.png Click to show the list of files on the server }

Once you have loaded a data file, you can view information about the loaded
data by clicking on the "i" icon in the toolbar, which appears only when data
has been loaded.  The image below, while it pertains to the wavelet source
rather than a loaded data file, illustrates the kind of information available
about a dataset in the information panel.  There are four sections within the
data information panel.  The section title "Statistics" shows the type of
data, the number of points/cells, and the amount of memory occupied.  The
"Data Arrays" section lists all the data arrays associated with data and
includes the numeric range of each array.  The "Bounds" section has
information about the geometric extent of the data in X, Y, and Z.  If the
data is time-varying, the "Time" section shows information about the
timesteps included.

{@img images/data-information.png Click to show the list of files on the server }

The following image links to a video illustrating the concepts discussed above.

[![Loading files](https://i.vimeocdn.com/video/505858970_150x84.jpg)](https://vimeo.com/118746895)

## Color management

After you have loaded a data file or a source and perhaps applied some filters,
you can change the representation of any pipeline component and manage how it
is colored showing the pipeline editor panel and interacting with the "Color
Management" section of the proxy property editor.  The image below shows an
example of the color management area of the proxy property editor.

{@img images/color-management.png Click to show the list of files on the server }

The "Representation" dropdown lets you change how the data is rendered (volume,
surface, wireframe, etc...).  Within the "Color" group, you can choose which
array to color by, and if it is a vector quantity, you can choose which
component to color by (or color by the magnitude of the vector).  Also within
the "Color" group, you can choose an initial color palette from the list of
available palettes.  Additionally, the bookmark icon doubles as a toggle
button for the color legend.  If the bookmark is filled, the scalar bar is
visible for the selected array, if the bookmark is empty, the scalar bar is
not visible for the selected array.

There are three additional buttons in the "Color Management" section which
become enabled on when coloring by an array (rather than "Solid color").
These buttons behave like tabs and show extra color-related controls, described
in the following subsections.

### Colormap editor

The colormap editor widget is used to customize the colormap after a preset
palette has been chosen as a starting point.  The image below shows the
color editor with the main buttons labelled.

{@img images/labelled-color-editor.png The color editor widget}

The color editor widget allows you to add and remove colors from the
color map, change existing colors, edit scalar values, and, when treating
values as categories, edit text annotations associated with color values.

Normally, all changes made using the color editor are immediately sent to
the server and the render view is updated to reflect them.  However, by
clicking the lock icon, you can toggle immediate mode off so that you may
change the color map without sending updates to the server.  When you are
not in immediate mode, you must click the apply button (the check mark icon)
to send your updated color map to the server.

When you add a new color/value to the color map list, it is normally inserted
right after the first element, and it is given a value half way between the
first and second values.  An exception is when you manually opt to interpret
values as categories by clicking the "A" icon.  In this case, there are
initially no color map entries, and the first two you add are given values
0 and 1 (and colors white and black, respectively).  At any time you can
choose to rescale the color map entries so that the values are evenly spaced
between the min and max values.

You may type in the scalar value text entry fields to assign a new scalar
value to the associated color.  When you do this, the entries will be
re-sorted so that they are always shown in increasing order of value.

You may also click on a color to assign a new color to the associated scalar
value.  Below the color map entry, you should see the swatches image appear,
along with R, G, and B edit fields, as shown in the image below.

{@img images/color-editor-swatches.png The color editor widget}

You can click on any color in the swatches images to use that color for the
associated scalar value in your color map.  You can also enter the red,
green, and blue values into the text edit fields.  In either case, your
choice of color should immediately appear in the color patch you clicked
to show the swatches.  If you are in immediate mode, the new color should
additionally be sent to the server and your render view updated.

If you have chosen to interpret values as categories, one final option
becomes available.  You can click the tags icon to toggle between showing
scalar values and text annotations in the color map entry text fields.  In
this way, you can edit both properties.  When you are editing text
annotations, the associated scalar value is displayed as a tooltip, and
when you are editing scalar values, the opposite is true.

Any time you select a preset palette, the interpret values as categories
option will be selected for you based on the type of the palette applied.
If a palette is configured as a color map which has indexed colors, the
interpret values as categories option will become selected.

To delete any entry from your color map list, simply click the trash can
icon to it's right.  You may delete any color map entries until there are
only two left, at which point the trash can icons become disabled.

### Opacity editor

The opacity editor widget is used to edit the scalar opacity function used
when doing volume rendering or when opacity mapping for surfaces has been
enabled.  The image below shows the opacity editor with the buttons labelled.

{@img images/labelled-opacity-editor.png The opacity editor widget}

The opacity function editor has two modes of operation, gaussian mode and
linear mode.  In gaussian mode, you click the circled plus icon to drop a
new gaussian, and as soon as your mouse enter the white plot area, you will
find you are already dragging the new gaussian around.  In linear mode, you
can either click the circled plus icon and move the mouse into the plot area,
or you can just click anywhere in the plot area to drop a new point.

The four buttons on the top left of the widget have the following behavior:

The first button toggles between gaussian and linear mode.  The most recent
set of gaussians and the most recent linear curve are always stored in
memory on the server side and associated with the array name by which you
are coloring.  For this reason you do not need to worry about switching
between modes and losing the function you had built in the other mode.

The second button puts you in a mode where as soon as your mouse enters the
plot area, you begin dragging a new gaussian or linear point, and as soon as
you click, the new curve or point is positioned.  Once the new curve or
point is added and initially positioned, it becomes the "active" one.

The third button deletes the currently active gaussian curve or linear point.

The fourth button deletes all the gaussian curves or linear points.  In linear
mode, however, the two endpoints cannot be deleted.  Be aware that in
gaussian mode, deleting all the curves results in a piecewise linear curve
equal to constant zero, and this will result in your data disappearing from
the 3D render window.

Regardless of which mode you are currently using, the three icons on the top
right have the same behavior.  Toggling opacity mapping for surfaces allows
you to render surfaces with opacity based on scalar values on the surface.
Interactive mode in the opacity function editor means that whenever you
release the mouse in the plot area, the scalar opacity function is
immediately sent to the server and a new image rendered.  This is useful for
more interactive manipulation of the function, but when volume rendering
very large data, this may not be what you want.  In that case, turn off
interactive mode so that you can edit the opacity function as much as you
want and then only send it to the server when you click the apply button
in the far upper right corner of the opacity function editor widget.

#### More on gaussian mode

When the opacity function editor is in gaussian mode, the idea is that you
will drop as many gaussians as you like, and then select one to make it
"active".  Then it will be plotted in blue and you will see four green
control points that control the shape of the gaussian.  You will then click
and drag the control points to adjust the five parameters which describe
the shape of the curve.

The upper green circle can be dragged to control both the center point and
the height of the active gaussian.  Both of the green circles along the
bottom do the same thing: control the width of the gaussian.  The green
diamond in the very center is a bit more interesting in that it controls
two bias parameters.

Drag the diamond left and right to get the curve to lean to the left or
right, respectively.  Drag it up and down to vary the curve smoothly from
a gaussian shape to a parabolic shape to a step function.  By playing with
the diamond control point, you can achieve a wide variety of shapes very
quickly.

#### More on linear mode

When the opacity function editor is in linear mode, there are always at
least two points that cannot be removed, these are at the left and right
endpoints of the plot area.  These points can be slid up and down however.
New points can only be added to the right of the left endpoint or to the
left of the right endpoint.  Clicking on any existing point causes it to
be drawn in green and makes that point active.  In linear mode, the points
are always sorted from left to right, so you are free to drag the active
point anywhere in the plot are with respect to other points.

The following image is a link pointing to a video which illustrates the use
of the opacity editor widget to do volume rendering.

[![Using the opacity function editor](https://i.vimeocdn.com/video/503537675_150x84.jpg)](https://vimeo.com/116991628)

### Color range editor

This small widget provides the means to rescale the coloring in several ways.
There are two textfields which allow entry of arbitrary min and max values, as
well as three buttons which apply range changes in different ways.  The
rightmost button (check icon) applies whatever color range has been entered in
the min/max textfields.  The middle button (clock icon) rescales over all
timesteps, while the leftmost button (arrows pointing out icon) rescales the
colors to the range of the currently visible data.

{@img images/range-editor-only.png The range editor widget }

## Working with time dependent data

When you load a data set that has timestep information, we provide the
VCR controls which can be toggled on and off in the toolbar by clicking
the clock icon.

{@img images/toggle-vcr-controls.png Toggle the vcr controls in the toolbar }

Turning on the vcr controls will add some controls to the toolbar, putting
them to the right of the preferences toggle button.

{@img images/vcr-controls.png The vcr controls in the toolbar }

Hopefully these buttons are fairly self-evident.  The play button in the
center allows you to animate through the time steps in a loop.  You can
still interact with the data using your mouse while the animation is
playing.

There are also forward and reverse buttons which will take you to the next
(or previous) timestep, as well as buttons which will take you to the last
(or first) timestep.

Additionally, there is a textfield in the vcr controls which shows the time
values at each time.  These correspond to the time values you will see in
the data information panel, under the "Time" section.

The image below links to a video illustrating the use of the VCR controls.

[![Working with Time Series](https://i.vimeocdn.com/video/506074785_150x84.jpg)](https://vimeo.com/118935634)

## Saving data

The ParaViewWeb Visualizer applications provides a way to save several kinds
of data from your visualization session to a configurable location on the
server.  To access the save options, first click on the disk icon on the
toolbar.

{@img images/show-saveopts-trans.png Click to show save data panel }

### Saving screenshots

The first tab in the save data panel provides a way to save the the rendered
image on the server.  You may provide an image size and a relative file path
where you would like the image to be saved.  The filename is going to be
appended to the directory on the server that was configured as the save
location, which by default is the same as the directory where data is
loaded from.

{@img images/save-screenshot.png Screenshot tab of the save data panel }

In the image above, note the refresh icon to the right of the "Desired
Resolution" label.  You can click this icon to update the resolution
text fields with the current viewport width and height.

Also, if you wish to save a screenshot locally rather than on the server,
just click the "Capture" button.  This will grab the current rendered
image and place it in the panel, where you can right-click it and choose
"Save As".

### Saving data files

You can also save the output of any filter or source on the server.  Simply
select the filter or source in the pipeline editor, and then open the save
options panel.  The "Data" tab of the save panel provides a text field
where you can enter the filename you wish to use when saving the output
data.

{@img images/save-data.png Data tab of the save data panel }

As a convenience, the data type of the currently selected filter
is shown to the right of the "Filename" label.  Also, the type of the data
is used to automatically update the file extension in the filename text
field so that the correct writer will be chosen when writing the data
file on the server.

### Saving state

And finally, the save panel also provides a means to save the current
state (this includes the topology of the pipeline and the values of all
filter and source properties).  The state file (saved with .pvsm
extension) can then be opened later on to get back to precisely the
same pipeline state that existed when the state was saved.

{@img images/save-state.png State tab of the save data panel }

## Changing preferences and settings

To show the preferences panel, click on the gears icon in the toolbar.

{@img images/show-prefs-trans.png Show the preferences panel }

The preferences panel is divided into two portions, as can be seen in the
image below.

{@img images/preferences.png The preferences panel }

The top portion of the preferences panel allows you to choose a rendering
mode, to choose whether or not to display rendering statistics in the
render view, and to choose whether or not to shut down the server when the
browser tab closes or navigates away from the application.  The available
rendering modes are

1. Remote
1. Local (VGL)
1. Local (Deprecated)

"Remote" rendering means rendering is done on the server side and only
the resulting images are delivered to the browser.  Both forms of
"Local" rendering involve geometry delivery and the use of WebGL to do
rendering in the browser.

The lower portion of the preferences panel exposes the RenderViewSettings
proxy properties.  This is where you can do things like choose Level Of
Detail threshold, turn on server-side annotations, etc.  Five properties
are shown here by default, but all of the render view settings can be
shown by clicking the "Toggle Advanced Properties" button.

As with the proxy properties editor available in the pipeline panel, you
must first make your desired changes to the render view settings, and when
finished, click the apply button.

The following image links to a short video illustrating the updating of
render view settings in ParaViewWeb Visualizer.

[![Updating global settings](https://i.vimeocdn.com/video/503538159_150x84.jpg)](https://vimeo.com/116991985)
