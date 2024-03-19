Property Hints    {#PropertyHints}
==============

This page documents *Property Hints*, which are xml tags accepted under *Hints*
for a *Property* element in Server-Manager configuration XMLs.

[TOC]

AllowRestoreDefaults
----------
Add a button to restore the property to default values.

Any repeatable vector property is displayed as a table that you update by adding
or removing values by default. However, you may want to restore default values,
so adding a button makes it easier than using the context menu.
In that case, use this property hint as follows.

    <IntVectorProperty ...>
      ...
      <Hints>
        <AllowRestoreDefaults />
      </Hints>
    </IntVectorProperty>

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

IsSelectable
------------
Add a checkbox for each element of the list and remove list update abilities.

A repeatable vector property is displayed as a table that you update by adding
or removing values by default. This hints changes its representation into a
checkable but non-editable table.
For now, can be used only with the timesteps domain that fills a double vector
property values.

    <DoubleVectorProperty ...>
      ...
      <Hints>
        <IsSelectable />
      </Hints>
    </DoubleVectorProperty>

ShowLabel
-----------------
Show vector label for double, int or string vector properties.

A repeatable vector property won't show the label of the property by default,
as the table can have component labels and takes more space. This hint forces
the label to show up in this case.

    <DoubleVectorProperty ...>
      ...
      <Hints>
        <ShowLabel />
      </Hints>
    </DoubleVectorProperty>

ShowComponentLabels
-----------------
Show vector component labels for double, int or string vector properties.

A multi-component vector property may represent several different types
of coordinates.  It may be useful to provide labels for each component's input
to designate which type of vector is being represented. Labels may also be
provided for multi-component string vector properties.

    <DoubleVectorProperty ...>
      ...
      <Hints>
        <ShowComponentLabels>
          <ComponentLabel component="0" label="X"/>
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

The optional attribute `unlink_if_modified` can be set to 1 if the link should
be broken if the user explicitly modifies the property. This is useful when
linking color-related properties with the active color palette, for example:

    <DoubleVectorProperty command="SetBackground"
                          default_values="0.329 0.349 0.427"
                          name="Background"
                          panel_widget="color_selector_with_palette"
                          number_of_elements="3">
      <Hints>
        <PropertyLink group="settings" proxy="ColorPalette" property="BackgroundColor" unlink_if_modified="1" />
      </Hints>
    </DoubleVectorProperty>


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
Specify height in rows for tabular/tree or combobox widgets.

Certain widgets that show a tabular widget of rows and columns i.e. any property widget
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

`WidgetHeight` hint can also be used on properties that use a `QComboBox` such as `EnumerationDomain`, `ProxyListDomains` or `StringListDomain`. It controls the maximum allowed number of item in the combobox before a scroll appear.

    <StringVectorProperty command="" name="...">
      <StringListDomain name="array_list">
        <RequiredProperties>
          <Property function="..." name="..."/>
        </RequiredProperties>
      </StringListDomain>
      <Hints>
        <!-- This tag sets the height of the QComboBox -->
        <WidgetHeight number_of_rows="5" />
      </Hints>
    </StringVectorProperty>

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
`<AcceptAnyFile/>` to accept any filename to export data, or
`<BrowseLocalFileSystem/>` to browse local file-system irrespective of the
connection type.

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

DoubleRangeSliderPropertyWidget
-------------------------------

Customize the `pqDoubleRangeSliderPropertyWidget` with these hints:

* `HideResetButton` - when an XML element with this name is present, the reset range button will not be shown
* `MinimumLabel` - the `text` attribute in this element will be used for the minimum slider widget label
* `MaximumLabel` - the `text` attribute in this element will be used for the maximum slider widget label

The snippet below shows these hints in use.

    <DoubleVectorProperty command="SetClippingLimits"
        default_values="0.8 1.2"
        name="ClippingLimits"
        panel_visibility="advanced"
        number_of_elements="2"
        panel_widget="double_range">
        <DoubleRangeDomain
            max="2.0"
            min="0.65"
            name="range" />
        <Hints>
          <MinimumLabel text="Near Clipping Limit"/>
          <MaximumLabel text="Far Clipping Limit" />
          <HideResetButton/>
        </Hints>
    </DoubleVectorProperty>

TextureSelectorPropertyWidget
-------------------------------

Customize the `pqTextureSelectorPropertyWidget` with the `TextureSelectorWidget` hint using the following attribute:

* `can_load_new` - if 0 then user will not be able to load new texture from the drop down. Default is 1.
* `check_tcoords` - if 1 then user will be able to use a texture only if the source has texture coordinates. Default is 0.
* `check_tangents` - if 1 then user will be able to use a texture only if the source has tangents. Default is 0.

The snippet below shows these hints in use.

```xml
<ProxyProperty command="SetTexture"
                  name="Texture"
                  panel_widget="texture_selector"
                  null_on_empty="1" >
    <ProxyGroupDomain name="groups">
      <Group name="myTextureGroup" />
    </ProxyGroupDomain>
    <Hints>
      <TextureSelectorWidget
        can_load_new="0"
        check_tcoords="1"/>
    </Hints>
  </ProxyProperty>
```

Widget
------

Enable specific features on a text entry widget.

For `StringVectorProperty` elements asking for a free input, the default
one-line text entry can be enhanced using one of the following hint attributes.

**Attribute `type`**

Set it to `multi_line` to allow line breaks, useful to enter a script.

The other possible value is `one_liner_wrapped`, when line breaks should be avoid in the text
but text wrapping is enabled for reading purpose. This also enable the integration with the
Expression Manager, to easily save and reuse entered property text.

**Attribute `syntax`**

Set it to `python` to ask for syntax highlighting when `type` is set to `multi_line` (`pygments` python module should be found).
With `one_liner_wrapped`, the tools button for Expression Manager are configured for `Python` expression type.

```xml
<StringVectorProperty command="SetScript"
                      name="Script"
                      number_of_elements="1"
                      panel_visibility="default">
  <Hints>
    <Widget type="multi_line" syntax="python" />
  </Hints>
</StringVectorProperty>
```
**Attribute `autocomplete`**

Set it to `python_calc` to enable Python Calculator style autocomplete on the text field. Completes many Numpy functions, fields and names from the paraview module. It can be used for widget with `type` attribute set to `one_liner_wrapped` or `multi_line`.

WarnOnPropertyChange
------

Display a warning message box when the *string* property has been changed. Set the `onlyonce` attribute to only show the message on the first property change.
The `Text` tag specifies the message box title and body text.

```xml
<Hints>
  <WarnOnPropertyChange onlyonce="true">
    <Text title="Message box title">
Message box message
  </Text>
  </WarnOnPropertyChange>
</Hints>
```
