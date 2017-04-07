Major API Changes             {#MajorAPIChanges}
=================

This page documents major API/design changes between different versions since we
started tracking these (starting after version 4.2).

Changes in 5.4
--------------

### Moved vtkAppendArcLength to VTK and exposed this filter in the UI.

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

Commit [afaf6a510](https://gitlab.kitware.com/paraview/paraview/commit/afaf6a510ecb872c49461cd850022817741e1558)
changes the internal widgets created in `pqViewFrame` to add a new `QFrame` named
**CentralWidgetFrame** around the rendering viewport. While this shouldn't break any
code, this will certainly break tests since the widgets have changed. The change to the testing
XML is fairly simple. Just add the **CentralWidgetFrame** to the widget hierarchy at the appropriate
location. See the original
[merge request](https://gitlab.kitware.com/paraview/paraview/merge_requests/167)
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
