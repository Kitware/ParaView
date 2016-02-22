Major API Changes             {#MajorAPIChanges}
=================

This page documents major API/design changes between different versions since we
started tracking these (starting after version 4.2).

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
