Property Hints    {#PropertyHints}
==============

This page documents *Property Hints*, which are xml tags accepted under *Hints*
for a *Property* element in Server-Manager configuration XMLs.

[TOC]

NoDefault
----------
Skip setting run-time defaults during initialization.

Any property with a domain that is dynamic i.e. depends on run-time
values such as data information or the information-only properties, often
resets the property value during initialization of the newly created proxy
(vtkSMParaViewPipelineController::PostInitializeProxy()). This, however, may
be undesirable and one may want the property to simply use the XML default.
In that case, use this property hint as follows.

    <IntVectorProperty ...>
      ...
      <Hints>
        <NoDefault />
      </Hints>
    </IntVectorProperty>

ComponentLabels
-----------------
Show vector component labels for double vector properties.

A multi-component double vector property may represent several different types
of coordinates.  It may be useful to provide labels for each component's input
to designate which type of vector is being represented.

    <DoubleVectorProperty ...>
      ...
      <Hints>
        <ShowComponentLabels>
          <ComponentLabel component="0" label="X:"/>
          ...
        </ShowComponentLabels>
      </Hints>
    </DoubleVectorProperty>

PlaceholderText
---------------
Put a PlaceHolder text in the Text entry widget.

For StringVectorProperty elements, one can suggest a place-holder text to use
in the UI using the `<PlaceholderText/>` tag.

    <StringVectorProperty...>
      ...
      <Hints>
        <PlaceholderText>Enter label</PlaceholderText>
      </Hints>
    </StringVectorProperty>

PropertyLink
------------
Link properties to existing proxies at initialization time.

PropertyLink hints is typically used to link a non-UI property to a
property on some other proxy known to exist when this proxy is created.
Consider this, we have a (settings, RenderViewSettings) proxy that created
when a new session is initialized in vtkSMParaViewPipelineController::InitializeSession().
Now, when a new (views, RenderView) proxy is created, we'd like a few properties
from the RenderView proxy to be linked to session wide instance of RenderViewSettings proxy.
That way, when the properties on RenderViewSettings proxy are changed, those on all
the RenderView proxy instances also change. We use `<PropertyLink/>` hint for such a case.
For example, consider the following:

    <IntVectorProperty command="SetUseOffscreenRenderingForScreenshots"
                        name="UseOffscreenRenderingForScreenshots"
                        number_of_elements="1"
                        default_values="0"
                        panel_visibility="never">
      <Hints>
        <PropertyLink group="settings"
                      proxy="RenderViewSettings"
                      property="UseOffscreenRenderingForScreenshots"/>
      </Hints>
    </IntVectorProperty>

Here, we want the UseOffscreenRenderingForScreenshots property linked with the corresponding
property on the RenderViewSettings proxy. When this tag is encountered by
vtkSMParaViewPipelineController::PostInitializeProxy(), it will try to locate an
existing proxy of registered as (settings, RenderViewSettings).
vtkSMParaViewPipelineController::InitializeSession() ensures that all proxies encountered in
\c settings group are created and registered using the same group. Thus, when the
vtkSMParaViewPipelineController::PostInitializeProxy() looks for (settings, RenderViewSettings)
proxy, it will find one and then be able to setup a property link to ensure that the property
on the RenderView proxy is kept in sync with the property on the RenderViewSettings proxy.

SelectionInput
--------------
Mark a input as a one that accepts a vtkSelection.

Sometimes an input to an algorithm is a vtkSelection, not any other dataset.
In ParaView, vtkSelection inputs are treated specially to allow the user
to copy active selection from the application. Also, it keeps the application
from prompting the user to pick the second input when the filter is created.
To indicate to the UI that this input is a selection input, use the
`<SelectionInput/>` hint.

    <InputProperty ...>
      ...
      <Hints>
        <SelectionInput />
      </Hints>
    </InputProperty>

WidgetHeight
------------
Specify height in rows for tabular/tree widgets.

Certain widgets that show a tablular widget of rows and columns i.e. any property widget
that uses a `pqTreeWidget` including the ones for  `ArrayListDomain`, `ArraySelectionDomain`,
`EnumerationDomain`, `CompositeTreeDomain`, respect this hint to setup the default size for the
tabular/tree widget.

    <IntVectorProperty command="..." name="...">
      <CompositeTreeDomain mode="all" name="tree">
        <RequiredProperties>
          <Property function="Input" name="Input" />
        </RequiredProperties>
      </CompositeTreeDomain>
      <Hints>
        <!-- This tag sets the height of the CompositeTreeDomain -->
        <WidgetHeight number_of_rows="20" />
      </Hints>
    </IntVectorProperty>

Expansion
---------
Expands all expandable items to the given depth in a tree widget.

Certain widgets that show a tree widget -ie. any property widget
that uses a `pqTreeWidget` including the ones for  `ArrayListDomain`, `ArraySelectionDomain`,
`EnumerationDomain`, `CompositeTreeDomain`- respect this hint to setup the default expansion depth
for the tree widget.

0 is the minimal expansion, -1 is expand all.

    <IntVectorProperty command="..." name="...">
      <CompositeTreeDomain mode="all" name="tree">
        <RequiredProperties>
          <Property function="Input" name="Input" />
        </RequiredProperties>
      </CompositeTreeDomain>
      <Hints>
        <!-- This tag sets the expansion depth of the CompositeTreeDomain -->
        <Expansion depth="3" />
      </Hints>
    </IntVectorProperty>

FileChooser
------------
Specify supported extensions to list for `pqFileChooserWidget` dialog.

For properties using FileListDomain to show a file chooser widget on the
Properties panel, sometimes we want to provide a list of extensions expected. In
that case, one can use this hint. Note, this is not intended for specifying
extensions that a reader supports. For that one uses the `<ReaderFactory>` hint
described in [ProxyHints](@ref ProxyHints). Multiple `FileChooser` hints may be
specified to show multiple extensions with different description texts.

By default, the file chooser widget will accept only existing files.
It is possible to add `<UseDirectoryName/>` in the `<Hints>` section to accept only directories,
or `<AcceptAnyFile/>` to accept any filename to export data.

    <StringVectorProperty animateable="0"
                          command="SetQFileName"
                          name="QFileName"
                          number_of_elements="1"
                          panel_visibility="default">
      <FileListDomain name="files" />
      <Documentation>This property specifies the .q (solution) file name for
        the PLOT3D reader.</Documentation>
      <Hints>
        <FileChooser extensions="q" file_description="Solution files" />
      </Hints>
    </StringVectorProperty>

OmitFromLoadAllVariables
------------
Specify that a dataset property be excluded when "Load All Variables" toggle is selected.

ParaView has a global toggle named "Load All Variables" that
automatically selects all variables in a dataset when loading a
file. This hint allows certain values to be omitted from that list
(e.g. sidesets, edgesets) so they will not be included by default. The
user can still manually select these values to be loaded.

    <StringVectorProperty command="SetSideSetArrayStatus"
                            element_types="2 0"
                            information_property="SideSetInfo"
                            name="SideSetArrayStatus"
                            number_of_elements_per_command="2"
                            repeat_command="1">
        <ArraySelectionDomain name="array_list">
          <RequiredProperties>
            <Property function="ArrayList"
                      name="SideSetInfo" />
          </RequiredProperties>
        </ArraySelectionDomain>
        <Documentation>An Exodus II file may define subsets of all the
        <i>boundaries</i>of all the elements in a file as sets in their own
        right. This property specifies which of those sets should be loaded.
        Variables, such as boundary conditions, may then be defined over these
        sets by specifying a single number per side. For example, a hexahedron
        has 18 sides: 6 faces and 12 edges. Any of these sides may be
        individually called out in a set and assigned a result value. The
        accompanying SideSetResultArrayStatus property specifies which
        variables defined over those sets should be loaded.</Documentation>
        <Hints>
          <OmitFromLoadAllVariables />
        </Hints>
    </StringVectorProperty>

ProxySelectionWidget
---------------------
Specify options to configure `pqProxySelectionWidget`, typically used for
proxy-properties with a proxy-list domain.

To hide the combo-box widget, add `visibility="0"` attribute.
To disable the combo-box widget, so that the user cannot change the selection,
add `enabled="0"` attribute.

    <ProxyProperty name="Format">
         <ProxyListDomain name="proxy_list">
           <Group name="screenshot_writers" />
         </ProxyListDomain>
         <Hints>
           <ProxySelectionWidget enabled="0" />
         </Hints>
    </ProxyProperty>


ArraySelectionWidget
---------------------

For a property that uses `pqArraySelectionWidget`, one can specify the icon to
use for the arrays listed using this hint. See
`pqArraySelectionWidget::setIconType` for supported icon types.

    <StringVectorProperty
          name="RowDataArrays"
          command="GetRowDataArraySelection"
          number_of_elements_per_command="1"
          repeat_command="1"
          si_class="vtkSIDataArraySelectionProperty">
          <ArrayListDomain name="array_list" input_domain_name="row_arrays">
            <RequiredProperties>
              <Property name="Input" function="Input" />
            </RequiredProperties>
          </ArrayListDomain>
          <Documentation>
            Select the row data arrays to pass through
          </Documentation>
          <Hints>
            <ArraySelectionWidget icon_type="row"/>
          </Hints>
      </StringVectorProperty>
    </SourceProxy>
