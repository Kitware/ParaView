Proxy Hints And Annotations {#ProxyHints}
===========================

This page documents *Proxy Hints*, which are XML tags accepted under *Hints*
for a *Proxy* element in the Server-Manager configuration XMLs.

WarnOnRepresentationChange
--------------------------
Warn the user on changing to a specific representation type.

For the motivation behind this hint, see BUG #15117.
This is used to indicate to the pqDisplayRepresentationWidget that the user must
be prompted with a *'Are you sure?'* if they manually switch to this
representation from the UI.

    <RepresentationProxy ...>
      ...
      <Hints>
        <WarnOnRepresentationChange value="Volume" />
      </Hints>
    </RepresentationProxy>

WarnOnCreate
------------
Warn the user when creating the filter or source in the UI.

The motivation behind this hint is to warn the user when executing filters
like **Temporal Statistics** filter since they can potentially take a long time
for large file series.

    <SourceProxy ...>
      ...
      <Hints>
        <WarnOnCreate title="Potentially slow operations">
          **Temporal Statistics** filter needs to process all timesteps
          available in your dataset and can potentially take a long time to complete.
          Do you want to continue?
        </WarnOnCreate>
    </SourceProxy>

ReaderFactory
-------------
Mark a proxy as reader proxy so that it's used to open files from the **File |
Open** dialog.

This hint is used to mark a proxy as a reader. It provides the ParaView
application with information about extensions supported by this reader.
**extensions** attribute to list the supported extensions e.g. "foo foo.bar" for
files named as `somename.foo` or `somename.foo.bar`.
**filename_patterns** attribute is used to list the filename patterns to match.
The format is similar to what one would use for `ls` or `dir` using wildcards e.g.
spcth\* to match spcta, spctb etc.

    <!-- using extensions -->
    <SourceProxy ...>
      ...
      <Hints>
        <ReaderFactory extensions="[space separated extensions w/o leading '.']"
            filename_patterns="[space separated filename patters (using wildcards)]"
            file_description="[user-friendly description]" />
      </Hints>
    </SourceProxy>

View
----
Specify the default view to use for showing the output produced by a
source/filter.

This hint is used to indicate the name of the view to use by default for showing
the output of this source/filter on first *Apply*. To specify the view type for
a specific output port, you can use the optional attribute **port**.

    <SourceProxy ...>
      ...
      <Hints>
        <View type="XYChartView" />
      </Hints>
    </SourceProxy>

In certain cases, in addition to showing the data in the "preferred" default
view, you may want to show the result in the current view as well e.g. when
**Plot Over Line** filter is created, while the result is shown in the
**XYChartView**, we want to show the line representation for the data being
plotted in the current Render View too. For that one can add the
`also_show_in_current_view` attribute to the `<View/>` tag.

    <SourceProxy ...>
      ...
      <Hints>
        <View type="XYChartView" also_show_in_current_view="1" />
      </Hints>
    </SourceProxy>

If the source/filter has more than 1 output ports, you can choose which port the
hint corresponds to by using the optional `port` attribute.

    <SourceProxy ...>
      ...
      <Hints>
        <View type="XYChartView" port="1" />
      </Hints>
    </SourceProxy>

The `<View/>` hint can also be used to prevent automatic display of an output in
any view. To do that, use the **None** type. Such source can then still be
displayed manually by toggling the visibility when an appropriate View is active.

    <SourceProxy ...>
      ...
      <Hints>
        <View type="None" />
      </Hints>
    </SourceProxy>

PipelineIcon
------------
Specify the pipeline icon view to use in the pipeline browser for this
source/filter proxy.

This hint is used to indicate the icon to use in the pipeline browser.
It can be either the full name of a qt resource icon or the name of a view type
for which an icon as already been defined.

    <SourceProxy ...>
      ...
      <Hints>
        <PipelineIcon name="XYChartView" />
      </Hints>
    </SourceProxy>

If the source/filter has more than 1 output port, you can choose which port the
hint corresponds to by using the optional `port` attribute.

    <SourceProxy ...>
      ...
      <Hints>
        <PipelineIcon name="XYChartView" port="1" />
      </Hints>
    </SourceProxy>

At the time of writing, the supported stock icon names were:
 * "SERVER"
 * "SECURE_SERVER"
 * "LINK"
 * "GEOMETRY"
 * "XYChartView"
 * "XYBarChartView"
 * "XYHistogramChartView"
 * "BoxChartView"
 * "SpreadSheetView"
 * "INDETERMINATE"
 * "None"
 * "EYEBALL"
 * "EYEBALL_GRAY"
 * "INSITU_EXTRACT"
 * "INSITU_EXTRACT_GRAY"
 * "INSITU_SERVER_RUNNING"
 * "INSITU_SERVER_PAUSED"
 * "INSITU_BREAKPOINT"
 * "INSITU_WRITER_PARAMETERS"
 * "CINEMA_MARK"

If the desired icon is not present in the list, it is possible to use a Qt resource icon name directly.

    <SourceProxy ...>
      ...
      <Hints>
        <PipelineIcon name=":/pqWidgets/Icons/pqCalculator.svg" />
      </Hints>
    </SourceProxy>

Available icons are visible in the sources of ParaView

If the desired icon is not present, it can be added, for example in the context of a plugin, using
GUI_RESOURCES in your ADD_PARAVIEW_PLUGIN macro, a .qrc file and your own icon file.

    <SourceProxy ...>
      ...
      <Hints>
        <PipelineIcon name=":/MyPluginQtResource/Icons/myIcon.png" />
      </Hints>
    </SourceProxy>

Plotable
--------
Mark output data as plotable in 2D chart views.

Chart views in ParaView e.g. **Bar Chart View**, **Line Chart View**, support
plotting data of any type. However, since such plots don't use distributed
rendering techniques, to avoid accidentally plotting large datasets, the plots
by default can only show sources/filters that produce `vtkTable` as the output.
If a source/filter doesn't produce  a `vtkTable`, but produces data that should
indeed be plotted by such views, one can use this hint.

    <SourceProxy ...>
      <Hints>
        <Plotable />
      </Hints>
    </SourceProxy>

RepresentationType
------------------
Specify the representation type to use by default when showing the output from a
source/filter in a particular view.

This hint is used to indicate the default representation type in any/all views.
The **view** attribute is optional. When not specified it matches all views.
Likewise, **port** attribute is optional. When not specified it matches all
output ports. The hints are processed in order. Hence when specifying multiple
Representation elements, start with most restrictive to least restrictive.

Note, this hint doesn't control which representation proxy gets created, but the
default value for the "Representation" property on the representation proxy
set using `vtkSMRepresentationProxy::SetRepresentationType()`.

    <SourceProxy ...>
      ...
      <Hints>
        <RepresentationType view="ComparativeRenderView" type="Surface" port="1"/>
        <RepresentationType view="RenderView" type="Wireframe" />
      </Hints>
    </SourceProxy>

Representation
--------------
Specify the representation proxy to create to show the output from a
source/filter in a particular view.

This hint is used to indicate the representation proxy to create to show the
output from a source/filter in a particular view, rather than letting the view
determine which representation proxy to create. This is rare. The more common
use-case of picking the default representation type is satisfied by
**RepresentationType** XML hint documented above.

The required **view** attribute specifies the view to which the hint applies and
should be set to the XML proxy name of the view. The required **type** attribute
specifies the XML proxy name for representation to create. The optional
**port** attribute can be used to limit the hint to specific output port.

    <SourceProxy ...>
      ...
      <Hints>
        <Representation view="RenderView" type="TextSourceRepresentation" />
      </Hints>
    </SourceProxy>

OutputPort
--------------------------
Set name and representation of a specific output port.

This hint is used to rename a specific output port
and also the type of representation to use.
Type can be either "text", "logo" or "progress".

The representation part of this hint may be deprecated soon.
Use Representation hint instead.

    <SourceProxy ...>
      ...
      <Hints>
        <OutputPort index="0"
                    name="Output-0"
                    type="text" />
      </Hints>
    </SourceProxy>

ShowProxyDocumentationInPanel
-----------------------------
Show an annotation label in the auto-generated panel generated using
pqProxyWidget.

This hint is used to indicate that the documentation for the proxy should be
shown in special label at the top of the panel generated for the proxy. This is
useful to show information to the user directly on the panel.

The ShowProxyDocumentationInPanel take one optional attribute **type**. The
possible values are:
1. *description*: (default) to use vtkSMDocumentation::GetDescription(),
2. *short_help*: to use vtkSMDocumentation::GetShortHelp(), and
3. *long_help*: to use vtkSMDocumentation::GetLongHelp().

    <SourceProxy ...>
      <Documentation>
        Some text that will be shown in the label.
      </Documentation>
      ...
      <Hints>
        <ShowProxyDocumentationInPanel type="description"/>
      </Hints>
    </SourceProxy>

ReloadFiles
-----------
Indicate the property on a reader to use to refresh (or reload) the reader to
make it re-read the data files.

This hint can be used for readers that support "smart refresh" to re-read files
when they are changed. Otherwise, ParaView will use the default mechanism which
simply recreated the reader thus forgoing any previous data cached by the
reader. The attribute **property** indicates the name of the property on the
reader proxy to invoke to make the reader refresh.

    <SourceProxy>
    ...
      <Hints>
        <ReloadFiles property="Refresh" />
      </Hints>
    </SourceProxy>

View Annotations
----------------
Views support the following annotations:

1. **ParaView::DetachedFromLayout**: **Deprecated in ParaView 5.7**.
This is no longer applicable as all views are created detached from layout by default.

Live Source
------------
Certain algorithms can generate new data autonomously, e.g. a source that reads
data from the network. The **LiveSource** hint allows ParaView to periodically
check with the algorithm if it has new data and update the application, if so.

For that, one simply adds a hint to the proxy as follows:

    <SourceProxy ...>
      ...
      <Hints>
        <LiveSource interval="100" />
      </Hints>
    </SourceProxy>

The algorithm subclass must have `bool GetNeedsUpdate()` method that returns
true if the algorithm needs update.

The `interval` attribute is optional (defaults to 100) and can be used to
provide a refresh rate in milliseconds.

ConnectToPortIndex
--------------------------
Connect to a specific port index.

This is used to connect a representation to an output port of a filter
other than the default (index = 0). Currently, this is only used to
modify the input port for the selection representation subproxy in
vtkSMPVRepresentationProxy.

    <RepresentationProxy ...>
      ...
      <Hints>
        <ConnectToPortIndex value="2" />
      </Hints>
    </RepresentationProxy>

ShowInMenu
--------------------------
Plugin specific hint to control how a filter/source is shown in menus

This is used **in plugins only** to configure how a source or filter
should be shown in the filters/sources menu. It enables to place them
into an existing category or a new category, as well as set the icon.

    <SourceProxy ...>
      ...
      <Hints>
        <ShowInMenu category="Category" icon=":/path/to/ressource/icon.png" />
      </Hints>
    </SourceProxy>
