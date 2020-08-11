Major API Changes             {#MajorAPIChanges}
=================

This page documents major API/design changes between different versions since we
started tracking these (starting after version 4.2).

Changes in 5.9
--------------

###Representations and Ordered Compositing for Render View###

We have refactored the code that manages how data redistribution is done when
ordered compositing is needed. Ordered compositing is used for rendering
translucent geometries or volume rendering in parallel. Previous implementation
used `vtkDistributedDataFilter` internally which required building of
`vtkPKdTree` for deciding how the data is distributed. The new implementation
uses `vtkRedistributeDataSetFilter` which supports arbitrary non-intersecting
bounding boxes. This also results is lots of code simplification. However, it
has resulted in API changes that will impact vtkPVDataRepresentation subclasses.

* `vtkPVRenderView::MarkAsRedistributable` and
  `vtkPVRenderView::SetOrderedCompositingInformation` have been removed. Instead one
  should use `vtkPVRenderView::SetOrderedCompositingConfiguration` which enables
  developers to specify exactly how the representation's data products
  participate in the data-redistribution stage.
* For representations based on `vtkImageVolumeRepresentation`, such
  representation no longer need to provide a `vtkExtentTranslator` subclass to
  the render view via `vtkPVRenderView::SetOrderedCompositingInformation` to
  enable the view to rebuild a KdTree. The view can now simply use the data
  bounds provided either via `vtkPVRenderView::SetGeometryBounds` or
  `vtkPVRenderView::SetOrderedCompositingInformation`.

Since most custom representations are based on vtkImageVolumeRepresentation,
vtkUnstructuredGridVolumeRepresentation or vtkGeometryRepresentation, developers
are advised to look at the changes to those representations to get a better feel
for how to use `vtkPVRenderView::SetOrderedCompositingInformation` instead of
the legacy APIs.

###Changes where ParaView applications settings files are stored on Windows

Due to a bug prior to 5.9 on Windows only, user settings JSON files (`<application>-UserSettings.json`)
written from ParaView-based applications were stored in the wrong directory on Windows.
Settings file are normally stored in a directory with the name of the organization,
but ParaView was storing them in a directory with the name of the application instead.
In ParaView 5.9, the `<application>-UserSettings.json` file is stored in the directory named
for the organization as expected.

###Run-time selection of OpenGL Widget rendering implementation###

We have enabled run-time selection of Widget rendering OpenGL implementation, by
default it is set to _native_ mode. However, this can be changed to _Stereo_ mode
in startup time by adding the flag `--stereo` to the _Paraview_ binary.

This also makes `pqQVTKWidget` not to extend from `pqQVTKWidgetBase`, instead,
extend from QWidget and contain (composition) either an instance of
`QVTKOpenGLNativeWidget` or `QVTKOpenGLStereoWidget`.

###Consistent font size in text source

Corrected a setting that resulted in unwanted scaling of the font size from the
*Text Source* that scaled according to the size of the *RenderView*.

Changes in 5.8
---------------

###Changes to caching in representation###

Previously, caching for animation playback was handled by representations using
`vtkPVCacheKeeper` filter in the representation pipelines. This had several
issues:

1. Each representations had to explicitly handle it, which was error-prone and
   often ended up not being used correctly.
2. Caching happened on the pre-data-movement i.e. during animation playback, the
   geometry still needed to be delivered to the rendering nodes.
3. Cache was incapable of distinguishing between pipelines that were modified
   during animation playback and those that weren't. Consequently, animations
   that simply moved the camera still ended up executing the representation
   pipelines for each time step.
4. Not all views / representations participated in caching, only render view
   did. Thus, if you had an animation that used a Spreadsheet view, for example,
   you'd get no benefit from caching.
5. Even with caching enabled, animation playback was slow.


With 5.8, we have refactored handling of caches in views and representations.
Major highlights/changes are as follows:

1. Caching is now the responsibility of the view. vtkPVDataDeliveryManager
   handles caching along with data movement for views. All views, not just
   render views, use vtkPVDataDeliveryManager (or subclass) to handle data
   movement from data-processing nodes to rendering nodes. Representations
   like `vtkChartRepresentation`, `vtkSpreadSheetRepresentation` can no longer
   do data movement in `RequestData` but instead have to follow the same pattern
   as `vtkGeometryRepresentation` and other render view representations to let
   the view (using vtkPVDataDeliveryManager) handle data movement.
   Consequently, if your `vtkPVDataRepresentation` subclass used a
   `vtkPVCacheKeeper` in your representation pipeline, you can simply remove it.
   Representation provide prepared data to the view using `vtkPVView::SetPiece`
   in `vtkPVView::REQUEST_UPDATE` pass, and obtain delivered data for rendering
   using `vtkPVView::GetDeliveredPiece` in `vtkPVView::REQUEST_RENDER`
   (`vtkPVRenderView::GetPieceProducer` is still supported, but we encourage developers
   to prefer the new `GetDeliveredPiece` API instead).
2. To ensure that the view creates a delivery manager of appropriate type, you
   may have to add the delivery manager as a subproxy, as shown in the example
   below:

    <SubProxy command="SetDeliveryManager">
       <Proxy name="DeliveryManager"
         proxygroup="delivery_managers"
         proxyname="ContextViewDeliveryManager"/>
     </SubProxy>

3. When cached data is available, `REQUEST_UPDATE` request will no longer result
   in a call to `vtkPVDataRepresentation::RequestData`. The representation will
   be provided the cached data during `REQUEST_RENDER` pass. Note, if using
   cache, you may get data that has older `MTime`, thus representation should
   not assume monotonically increasing m-times for delivered data. Rendering
   pipelines in representations can thus rely on the mtime and data pointer to
   build their own rendering objects cache, if needed.
4. Representations always have to let the view do data transfer and obtain the
   data to render from the view and never simply connect the data prepared in
   `RequestData` to mappers, for example. This was acceptable in certain cases
   before, but not longer supported since `RequestData` will not get called
   when cache data is being used.

###Plugin loading (including auto loaded plugins)###

With this version, ParaView is now formalizing that all plugins, including the
ones that are flagged as "auto-load" are loaded **after** the main window has
been instantiated. This ensures that plugin developers can assume existence of a
main window irrespective of whether the plugin was auto-loaded or loaded after
the application has started.


###GlobalPropertyLink XML hints###

The implementation for settings proxies and the proxy for color palette has been
combined thus removing the need for a separate `vtkSMGlobalPropertiesProxy`
class. The color-palette proxy is now same as other settings proxies and is
registered under `settings` group and not `global_properties` group. To simply
XML hints to reflect this change, we have deprecated `GlobalPropertyLink` hint.
Following XML snippet shows how to change existing `GlobalPropertyLink` element
to `PropertyLink`.

    <!-- OLD XML -->
    <DoubleVectorProperty name="GridColor"
                          number_of_elements="3" ... >
      ...
      <Hints>
        <GlobalPropertyLink type="ColorPalette" property="ForegroundColor" />
      </Hints>
    </DoubleVectorProperty>

    <!-- NEW XML -->
    <DoubleVectorProperty name="GridColor"
                          number_of_elements="3" ... >
      ...
      <Hints>
        <PropertyLink group="settings" proxy="ColorPalette" property="ForegroundColor"
          unlink_if_modified="1" />
      </Hints>
    </DoubleVectorProperty>

Note, the `PropertyLink` hint was already present and used in earlier versions
for linking with properties on other settings proxies. We simply are following
the same pattern for color palette. The new `unlink_if_modified` attribute
ensures that the link with the settings proxy is broken if the user explicitly
modifies the property.

Changes in 5.7
--------------

###vtkSMFieldDataDomain in array selection properties###

Previously, `vtkSMFieldDataDomain` was added to `vtkSMStringVectorProperty`
instances that allowed user to choose the array to process. This is no longer needed
or supported. Simply remove the `vtkSMFieldDataDomain` from the XML for such properties.
Typically such properties have `vtkSMArrayListDomain` and that domain alone is
sufficient to enable selection of array name and its association.

###Changes to vtkSMFieldDataDomain###

vtkSMFieldDataDomain no longer updates the domain based on the type of the
input. The domain will always list the attribute types known to VTK/ParaView. As
a result, XML attributes `force_point_cell_data` and
`disable_update_domain_entries` for the XML definition of vtkSMFieldDataDomain
have been deprecated and should simply be removed from the XML as they are no
longer relevant.

##pqDoubleLineEdit API changes##

`pqDoubleLineEdit` now is simply a `pqLineEdit` with no signficant API differences
as far as setting and getting text values is concerned. This implies, however, that
tests that were updated in ParaView 5.6 to use `set_full_precision_text` instead of
`set_string` must now be reverted back to use `set_string`.

The following command was adequate for ParaView test changes:
`git grep -l set_full_ | xargs sed -i 's/set_full_precision_text/set_string/g'`.

##Views and Layouts##

Previously, when a new view is created ParaView automatically associated it with
a layout. This is no longer the case. Associating a view with a layout is now an
explicit action.
`vtkSMParaViewPipelineControllerWithRendering::AssignViewToLayout` may be used
to help assign a view to an available layout (or create a new one, if none
exists). This was essential for addressing several layout related issues such
as paraview/paraview#18964, and paraview/paraview#18963.


Changes in 5.6
--------------

###vtkPVDataInformation and vtkTable###

Previously, `vtkPVDataInformation` would accumulate row and column counts when
gathering information from a vtkTable in the `vtkPVDataInformation::NumberOfCells`
variable. This was incorrect and PraView 5.6 fixes this. For `vtkTable`,
`vtkPVDataInformation::GetNumberOfCells` will now return 0.

Changes in 5.5
--------------


###Replace pqLineEdit with pqDoubleLineEdit for viewing and editing double properties###

Tests recorded prior to this version need to be updated to:

* reference `DoubleLineEdit` instead of `LineEdit`
* use `set_full_precision_text` command instead of `set_string`

Note that the changes must only be applied to properties of type double. This means
that systematic search and replace can not be directly used.

See #17966

###Changes to Python shell###

`pqPythonShell` now is a fairly self contained class. One no longer has to go
through `pqPythonManager` or `pqPythonDialog` to create it. `pqPythonDialog` has
been deprecated. `pqPythonManager` has been relegated to a helper class to
support macros (together with `pqPythonMacroSupervisor`). The Python macro
infrastructure may be revamped in future, but is left unchanged for now.

If you want a Python shell in your application, simply create an instance of
`pqPythonShell` and add it where ever you choose. Multiple instances are also
supported.

###Changes to offscreen rendering options###

ParaView executables now automatically choose to create on-screen or off-screen
render windows for rendering based on the executable type and current
configuration. As a result **"UseOffscreenRendering"** property on views has
been removed to avoid conflicting with the new approach. Executables have new
command line arguments: `--force-offscreen-rendering` and
`--force-onscreen-rendering` that can be used to override the default behavior
for the process.

###Changes to vtkArrayCalculator
`vtkArrayCalculator::SetAttributeMode` is deprecated in favor of `vtkArrayCalculator::SetAttributeType`
which takes vtkDataObject attribute modes instead of custom constants as its parameter value.
The ParaView calculator filter's AttributeMode property's values changed as a result of this.

###Changes to vtkSMViewProxy::CaptureWindowInternal###

`vtkSMViewProxy::CaptureWindowInternal` now takes a 2-component magnification
factor rather than a single component. That allows for more accurate target image
resolution than before (see #17567).

###Replaced LockScalarRange property in PVLookupTable with AutomaticRescaleRangeMode###

The `LockScalarRange` property in the `PVLookupTable` proxy has been removed. It
has been replaced by the `AutomaticRescaleRangeMode` property. When
`AutomaticRescaleRangeMode` is set to `vtkSMTransferFunctionManager::Never`,
the transfer function minimum and maximum value is never updated, no matter what
event occurs. This corresponds to when `LockScalarRange` was set to "on" in
previous versions of ParaView. When `AutomaticRescaleRangeMode` is set to a
different option, that option governs how and when the transfer function is
reset. This option overrides whichever option is set in the `GeneralSettings`
property `TransferFunctionResetMode`.

The `TransferFunctionResetMode` property still exists, but it has been slightly
repurposed. No longer does it control the range reset mode for ALL transfer
functions. Instead, it serves the following functions:

* determining the `AutomaticRescaleRangeMode` when a lookup table is created, and

* determining what the `AutomaticRescaleRangeMode` should be when the data array
  range is updated to the data range of the selected representation

As part of this change, the enum values

    GROW_ON_APPLY
    GROW_ON_APPLY_AND_TIMESTEP
    RESET_ON_APPLY
    RESET_ON_APPLY_AND_TIMESTEP

have been moved from `vtkPVGeneralSettings.h` to `vtkSMTransferFunctionManager.h`.

###Deprecated pqDisplayPolicy###

`pqDisplayPolicy`, a class that has been unofficially deprecated since
`vtkSMParaViewPipelineControllerWithRendering` was introduced is now officially
deprecated and will be removed in future. Use
`vtkSMParaViewPipelineControllerWithRendering` to show/hide data in views
instead of `pqDisplayPolicy`.

###Changes to pqParaViewMenuBuilders###

The signature of `pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(QWidget&)`
has changed to `pqParaViewMenuBuilders::buildPipelineBrowserContextMenu(QMenu&)` and
now requires a `QMenu` to populate rather than populating the implicitly created context
menu in a widget.

Note that the `contextMenuPolicy` set in the widget providing the context menu must be
set to `Qt::DefaultContextMenu` for the context menu to appear. in `pqPipelineBrowserWidget`,
this used to be set to `Qt::ActionsContextMenu`.

Changes in 5.4
--------------

###Moved vtkAppendArcLength to VTK and exposed this filter in the UI.

As a result of this change, `vtkAppendArcLength` was moved from
the `internal_filters` to the `filters` group.

###Changes to Save Screenshot / Save Animation###

`pqSaveSnapshotDialog` is removed. This class was used by
`pqSaveScreenshotReaction` to present user with options to control the saved
image. This is now done using `pqProxyWidgetDialog` for the **(misc,
SaveScreenshot)** proxy. See `pqSaveScreenshotReaction` for details.

`pqStereoModeHelper` has been removed. It was to help save/restore stereo mode
for views for saving screenshots and animations. All of that logic is now
encapsulated in `vtkSMSaveScreenshotProxy`.

`pqMultiViewWidget::captureImage`, `pqMultiViewWidget::writeImage`,
`pqTabbedMultiViewWidget::captureImage`, `pqTabbedMultiViewWidget::writeImage`,
`pqView::captureImage`, `pqView::writeImage` have been deprecated and will be removed in
future releases. Saving and capturing images now goes through pqSaveScreenshotReaction
(or **vtkSMScreenshotOptions** proxy).

Changes in 5.3
--------------

###Removed pqRescaleCustomScalarRangeReaction, pqRescaleVisibleScalarRangeReaction###

`pqRescaleCustomScalarRangeReaction` and `pqRescaleVisibleScalarRangeReaction`
have been removed and replaced by the more
generic `pqResetScalarRangeReaction`. Set mode
to`pqResetScalarRangeReaction::CUSTOM` when instantiating
`pqResetScalarRangeReaction` to make it behave as
`pqRescaleCustomScalarRangeReaction`. The mode can be set to
`pqResetScalarRangeReaction::VISIBLE`, for
`pqRescaleVisibleScalarRangeReaction`.

###Dropped support for pqProxyPanel and subclasses (Legacy Panels)###

`pqProxyPanel` and its subclasses that formed the default mechanism for providing
custom panels for filters, displays, views, etc. before ParaView 3.98 has now
been removed. Refer to the
[wiki](http://www.paraview.org/Wiki/ParaView/Properties_Panel) for getting
oriented with the currently supported way for customizing the properties panel.
This also removes plugin macros `ADD_PARAVIEW_OBJECT_PANEL()`, and
`ADD_PARAVIEW_DISPLAY_PANEL()`.

###Qt 5 Support (replacing QVTKWidget)###

ParaView has switched over to using Qt 5 by default. While for most cases the
application code should be unaffected, if your app was directly creating (or
using) `QVTKWidget` however, you will need to change it. When
building with Qt 5, you are expected to use `QVTKOpenGLWidget` instead of
`QVTKWidget` (which is still needed when building with Qt 4).
Since ParaView supports building with Qt 4 and 5 for this release, to make it
easier, we have the following solution.

A new header **pqQVTKWidgetBase.h** defines a new typedef `pqQVTKWidgetBase`.
Include this header in your code and replace `QVTKWidget` with `pqQVTKWidgetBase`.
`pqQVTKWidgetBase` gets typedef'ed to `QVTKWidget` for Qt 4 builds and to
`QVTKOpenGLWidget` for Qt 5 builds.

If your existing code was creating and setting up a new instance of
`QVTKWidget`, you need to change it further. QVTKOpenGLWidget cannot work with
any `vtkRenderWindow` (as was the case with QVTKWidget),
it instead needs a `vtkGenericRenderWindow` (see the documentation
for `QVTKOpenGLWidget` for details). To make that easier, **pqQVTKWidgetBase.h**
defines another typedef `pqQVTKWidgetBaseRenderWindowType`. The code to create
and setup QVTKWidget can then be changed as follows:

    vtkNew<pqQVTKWidgetBaseRenderWindowType> renWindow;
    pqQVTKWidgetBase* widget = new pqQVTKWidgetBase(...);
    widget->SetRenderWindow(renWindow.Get());

If you are using a `vtkView` to create the viewport. You can set that up as
follows:

    vtkNew<pqQVTKWidgetBaseRenderWindowType> renWindow;
    pqQVTKWidgetBase* widget = new pqQVTKWidgetBase(...);
    widget->SetRenderWindow(renWindow.Get());

    vtkNew<vtkContextView> view;
    view->SetRenderWindow(renWindow.Get());

If your application supports Qt 5 alone, when you should directly use
`QVTKOpenGLWidget`. `pqQVTKWidgetBase` is a temporary solution to support
building with Qt 5 and Qt 4. Qt 4 support will be deprecated and remove in
future releases.

###Removed ctkRangeSlider and ctkDoubleRangeSlider###

These classes were not used by ParaView. Use pqDoubleRangeSliderPropertyWidget
instead.

Changes in 5.2
--------------

###Removed pqStandardArrayColorMapsBehavior###

Setting the default colormaps from a Qt Behavior was causing inconsistent results
between the ParaView UI and pvpython/pvbatch.  Adding these default settings was
moved into the server manager.  To override these defaults, use
vtkSMSettings::AddCollectionFromX with a priority greater than 0.  The settings
affected are the default colormap for arrays with names vtkBlockColors and
AtomicNumbers.  With these settings moved, the Qt behavior
pqStandardArrayColorMapsBehavior no longer does anything and so it has been removed.

###Qt dependencies###

Starting with 5.2, ParaView natively supports Qt 4 and Qt 5. To simplify writing
code with either dependency, we now have a new CMake file `ParaViewQt.cmake`
that gets included by `PARAVIEW_USE_FILE`. This provides new macros that be used
to find qt (`pv_find_package_qt`), wrap cpp (`pv_qt_wrap_cpp`), ui
(`pv_qt_wrap_ui`), or add resources (`pv_qt_add_resources`) instead of using
`qt4_` or `qt4_` specific versions based on whether the app is being built with
Qt4 or Qt5. `pv_find_package_qt` accepts optional `QT5_COMPONENTS` and
`QT4_COMPONENTS` which can be used to list the Qt component dependencies for
each of the versions. e.g.

    include(ParaViewQt) # generally not needed, since auto-included
    pv_find_package_qt(qt_targets
      QT4_COMPONENTS QtGui
      QT5_COMPONENTS Widgets)

    pv_qt_wrap_cpp(moc_files ${headers})
    pv_qt_wrap_ui(ui_files ${uis})

    ...
    target_link_libraries(${target} LINK_PRIVATE ${qt_targets})

###Multiple input ports with vtkPythonProgrammableFilter###

vtkPythonProgrammableFilter can now accept multiple input ports if the number
of input ports is defined in the XML plugin file with the *input_ports* attribute.
The different input ports are then defined with InputProperty having each a
different *port_index*:

    <SourceProxy name="Name" class="vtkPythonProgrammableFilter" label="label" input_ports="2">
       <InputProperty name="Source" command="SetInputConnection" port_index="0">
         [...]
       </InputProperty>
       <InputProperty name="Target" command="SetInputConnection" port_index="1">
         [...]
       </InputProperty>
    </SourceProxy>

Changes in 5.1
--------------

###Removed Cube Axes###

Cube axes, including support in UI, Python as well as the related ParaView
specific classes e.g. `vtkCubeAxesRepresentation`, `pqCubeAxesEditorDialog`, and
`pqCubeAxesPropertyWidget` have been removed. The cube
axes was replaced by a generally preferred axes annotation implementation called
**Axes Grid**. While the two are not compatible (API-wise or visually), using
Axes Grid generates a more pleasing visualization.

###Removed `pqWriterDialog`###

`pqWriterDialog` was used by `pqSaveDataReaction` to show a dialog for the user to
change the writer's properties. However, since the class was added, we have
created a new, more generic, pqProxyWidgetDialog that also allows the user to
save his choices to the application settings. `pqSaveDataReaction` now simply
uses `pqProxyWidgetDialog` instead of `pqWriterDialog`. `pqWriterDialog` class
has been removed.

###Refactored 3DWidget panels###

3DWidget panels were subclasses of `pqObjectPanel`, the Properties panel
hierarchy that has been deprecated since 4.0. This version finally drops support
for these old 3D widget panels. The new implementation simply uses the
`pqPropertyWidget` infrastructure. A 3D widget panel is nothing more than a custom
`pqPropertyWidget` subclass for a property-group (`vtkSMPropertyGroup`) which
creates an interactive widget to be shown in the active view to help the users
change the values for properties in the property-group using this interactive
widget, in addition to standard Qt-based UI elements.

If you have a filter or proxy indicating that the Properties panel use one of
the standard 3D widgets for controlling certain properties on that proxy, you
specified that using `<Hints>` in the Proxy's XML definition.

For example, the **(implicit_functions, Plane)** proxy requested that
`pqImplicitPlaneWidget` be used using the following XML hints.

    <Proxy class="vtkPVPlane" name="Plane">
      <InputProperty is_internal="1" name="Input" />
      <DoubleVectorProperty command="SetOrigin"
                            default_values="0.0 0.0 0.0"
                            name="Origin"
                            number_of_elements="3">
      ...
      </DoubleVectorProperty>
      <DoubleVectorProperty command="SetNormal"
                            default_values="1.0 0.0 0.0"
                            name="Normal"
                            number_of_elements="3">
      ...
      </DoubleVectorProperty>
      <DoubleVectorProperty animateable="1"
                            command="SetOffset"
                            default_values="0.0"
                            name="Offset"
                            number_of_elements="1">
      </DoubleVectorProperty>
      <Hints>
        <PropertyGroup type="Plane">
          <Property function="Origin" name="Origin" />
          <Property function="Normal" name="Normal" />
        </PropertyGroup>
        <ProxyList>
          <Link name="Input" with_property="Input" />
        </ProxyList>
      </Hints>
    </Proxy>

Since 3D widget panels are now simply custom `pqPropertyWidget` subclasses for a
property-group, this code changes are follows:

    <Proxy class="vtkPVPlane" name="Plane">
      <InputProperty is_internal="1" name="Input" />
      <DoubleVectorProperty command="SetOrigin"
                            default_values="0.0 0.0 0.0"
                            name="Origin"
                            number_of_elements="3">
      ...
      </DoubleVectorProperty>
      <DoubleVectorProperty command="SetNormal"
                            default_values="1.0 0.0 0.0"
                            name="Normal"
                            number_of_elements="3">
      ...
      </DoubleVectorProperty>
      <DoubleVectorProperty animateable="1"
                            command="SetOffset"
                            default_values="0.0"
                            name="Offset"
                            number_of_elements="1">
      </DoubleVectorProperty>
      <PropertyGroup label="Plane Parameters" panel_widget="InteractivePlane">
        <Property function="Origin" name="Origin" />
        <Property function="Normal" name="Normal" />
        <Property function="Input"  name="Input" />
      </PropertyGroup>
      <Hints>
        <ProxyList>
          <Link name="Input" with_property="Input" />
        </ProxyList>
      </Hints>
    </Proxy>

Things to note:

- The `<PropertyGroup>` tag is no longer specified under `<Hints>`.
- The `panel_widget` attribute is used to indicate which custom widget to create
for this property group.

The functions for properties in the group supported by each type 3D widget (or
interactive widget as we now will call them) can be found by looking at the
documentation of each of the custom widget listed below. For the most past,
these have remained unchanged for previous function names.

Available custom property-widgets, the interactive widget they use and the
obsolete 3DWidget they correspond to are given in the table below.

New Widget | panel_widget | Interactive Widget used | Obsolete 3DWidget name
-----------|--------------|-------------------------|-------------------------------------
pqBoxPropertyWidget |InteractiveBox | BoxWidgetRepresentation | pqBoxWidget
pqHandlePropertyWidget | InteractiveHandle | HandleWidgetRepresentation | pqHandleWidget, pqPointSourceWidget
pqImplicitPlanePropertyWidget | InteractivePlane | ImplicitPlaneWidgetRepresentation |  pqImplicitPlaneWidget
pqLinePropertyWidget | InteractiveLine | LineSourceWidgetRepresentation | pqLineWidget, pqDistanceWidget, pqLineSourceWidget
pqSpherePropertyWidget | InteractiveSphere | SphereWidgetRepresentation | pqSphereWidget, pqOrbitWidget
pqSplinePropertyWidget | InteractiveSpline or InteractivePolyLine | SplineWidgetRepresentation or PolyLineWidgetRepresentation | pqSplineWidget, pqPolyLineWidget

###Changes to vtkDataArray Subclasses and In-Situ Arrays###

vtkDataArray can now support alternate memory layouts via the
vtkAOSDataArrayTemplate or other subclasses of
vtkGenericDataArray. While the older (and slower) vtkTypedDataArray
and vtkMappedDataArray interfaces still exist, they should not be used
and direct vtkGenericDataArray subclasses are preferred. See
http://www.vtk.org/Wiki/VTK/Tutorials/DataArrays for more details on
working with the new arrays.

The following methods have been deprecated in vtkAOSDataArray:

~~~{.cpp}
void GetTupleValue(vtkIdType tupleIdx, ValueType *tuple)
void SetTupleValue(vtkIdType tupleIdx, const ValueType *tuple)
void InsertTupleValue(vtkIdType tupleIdx, const ValueType *tuple)
vtkIdType InsertNextTupleValue(const ValueType *tuple)
~~~

The replacements use `TypedTuple` in place of `TupleValue`.

Changes in 5.0.1
----------------

###Changes to vtkSMInputArrayDomain###

vtkSMInputArrayDomain has changed the meaning of **"any"** attribute type. It
now includes field data arrays. To exclude field data arrays from the field
selection, change this to  **"any-except-field"**. This is also the default for
vtkSMInputArrayDomain, hence simply removing the `attribute_type` field is also
an acceptable solution and is the recommended approach so that the XML can work
without changes in earlier versions of ParaView as well.

Changes in 5.0
--------------

###Changed *Source* property to **GlyphType** on certain representations###

*Source* property on representations, which was not exposed in the UI has been
changed to *GlyphType* to better match its role. Since this property was not
exposed earlier, this change should not affect any users except custom
applications that explicitly used this property.


###Changes to caching infrastructure in **vtkPVDataRepresentation** and subclasses###

To avoid extra work and sync issues when propagating **UseCache** and
**CacheKey** flags from a vtkPVView to vtkPVDataRepresentation,
vtkPVDataRepresentation was updated to directly obtain thet values for these
flags from the view. To enable this, vtkPVDataRepresentation saves a weak
reference to the View it's being added to in vtkPVDataRepresentation::AddToView
and vtkPVDataRepresentation::RemoveFromView. Subclasses of
vtkPVDataRepresentation didn't consistently call the superclass implementation,
hence any representation subclass should ensure that it calls the superclass
implementations of AddToView and RemoveFromView for this to work correctly.
vtkPVView::AddRepresentationInternal checks for this and will raise a runtime
error if a representaiton is encountered that didn't properly set its View.

The above change also makes **vtkPVDataRepresentation::SetUseCache** and
**vtkPVDataRepresentation::SetCacheKey** obsolete. Subclasses no longer need to
provide any implementation for these methods and they should simply be removed.

Changes in 4.4
--------------

###Refactored pqView widget creation mechanisms###

pqView now adds a pure virtual method pqView::createWidget(). Subclasses should
override that method to create the QWidget in which the view is *rendered*. In
the past the subclasses implemented their own mechanisms to create the widget
and return it when `getWidget()` was called the first time.

`pqView::getWidget()` is now deprecated. Users should use pqView::widget()
instead. This method internally calls the pqView::createWidget() when
appropriate.

###Removed vtkPVGenericRenderWindowInteractor, vtkPVRenderViewProxy###

ParaView was subclassing vtkRenderWindowInteractor to create
`vtkPVGenericRenderWindowInteractor` to handle interaction. That piece of code
was potentially derived from an older implementation of
vtkRenderWindowInteractor and hence did what it did. Current implementation of
vtkRenderWindowInteractor lets the vtkInteractionStyle (and subclasses) do all
the heavy lifting. ParaView did that to some extent (since it has a
vtkPVInteractorStyle), but will was relying on
`vtkPVGenericRenderWindowInteractor`, `vtkPVRenderViewProxy` to propagate
interaction/still renders and other things. This has been refactored. ParaView
no longer uses a special vtkRenderWindowInteractor. All logic is handled by
observers on the standard vtkRenderWindowInteractor.

This change was done to make it easier to enable interactivity in `pvpython`.

See also: vtkSMRenderViewProxy::SetupInteractor(). Subclasses of pqView now pass
the interactor created by QVTKWidget to this method to initialize it.

See also: vtkSMViewProxyInteractorHelper.

###Refactored Color Preset Management (and UI)###

The color preset management has been completely refactored for this release.
This makes presets accessible to Python clients.
`vtkSMTransferFunctionPresets` is the class that manages the presets in the
*ServerManager* layer.

On the UI side, `pqColorPresetModel`, `pqColorPresetManager`, and
`pqColorMapModel` no longer exist. They have been replaced by `pqPresetDialog`.
Since the UI is completely redesigned, tests in custom applications
that used the color preset dialog will need to be updated as well.

The preset themselves are now serialized as JSON in the same form as the
settings JSON. ColorMaps in legacy XML format are still loadable from the UI. At
the same time, a tool `vtkLegacyColorMapXMLToJSON` is available to convert such
XMLs to JSON.

###Changes to `pqViewFrame`###

Commit [afaf6a510](https://gitlab.kitware.com/paraview/paraview/-/commit/afaf6a510ecb872c49461cd850022817741e1558)
changes the internal widgets created in `pqViewFrame` to add a new `QFrame` named
**CentralWidgetFrame** around the rendering viewport. While this shouldn't break any
code, this will certainly break tests since the widgets have changed. The change to the testing
XML is fairly simple. Just add the **CentralWidgetFrame** to the widget hierarchy at the appropriate
location. See the original
[merge request](https://gitlab.kitware.com/paraview/paraview/-/merge_requests/167)
for details.

###Changes to `vtkSMProxyIterator`###

`vtkSMProxyIterator::Begin(const char* groupName)` now additionally
sets the iterator mode to iterate over one group. This will likely
break any code that uses this method, since the iterated data list will
change. To recover the original functionality of the iterator, simply call
`vtkSMProxyIterator::SetModeToAll()` after calling
`vtkSMProxyIterator::Begin(const char* groupName)`.

Changes in 4.3
--------------

###Replaced `pqCurrentTimeToolbar` with `pqAnimationTimeWidget`###

`pqCurrentTimeToolbar` was a `QToolbar` subclass designed to be used in an
application to allow the users to view/change the animation time. We
reimplemented this class as `pqAnimationTimeWidget` making it a `QWidget`
instead of `QToolbar` to make it possible to use that in other widgets e.g.
`pqTimeInspectorWidget`. `pqAnimationTimeToolbar` was also changed to use
`pqAnimationTimeWidget` instead of `pqCurrentTimeToolbar`.

Besides just being a cleaner implementation (since it uses `pqPropertyLinks`
based linking between ServerManager and UI), `pqAnimationTimeWidget` also allows
applications to change the animation *play mode*, if desired. The rest of the
behavior of this class is similar to `pqCurrentTimeToolbar` and hence should not
affect applications (besides need to update any tests since the widget names
have now changed).

###Removed the need for plugins to specify a GUI_RESOURCE_FILE###

It is cleaner and easier to update one xml file for each plugin.
Hints in the server manager xml file already perform the same functionality and
the GUI xml file was deprecated.  Sources are now added to the **Sources** menu
and filters are added to the **Filters** menu when they are detected in the server
xml file.  To place sources/filters in a category, add the following hint to the
Proxy XML element (`category` and `icon` attributes are optional):

    <SourceProxy ...>
      ...
      <Hints>
        <ShowInMenu category="<category-name>" icon="<qt-image-resource>" />
      </Hints>
    </SourceProxy>

The ability to add filters to the sources menu (as done by a few plugins) is
removed by this change.

This change also modifies how readers are detected.  Readers now must provide a
hint to the ReaderManager in the server xml file to be detected as readers rather
than sources.  The backwards compatibility behavior that assumed any source with
a FileName attribute was a reader has been removed.

###Removed pqActiveView (use pqActiveObjects instead)###

`pqActiveView` was a long deprecated class which internally indeed used
pqActiveObjects. `pqActiveView` has now been removed. Any code using
`pqActiveView` can switch to using pqActiveObjects with very few code changes.
