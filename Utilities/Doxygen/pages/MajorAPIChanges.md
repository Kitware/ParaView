Major API Changes             {#MajorAPIChanges}
=================

This page documents major API/design changes between different versions since we
started tracking these (starting after version 4.2).

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
was potentially derrived from an older implementation of
vtkRenderWindowInteractor and hence did what it did. Current implementation of
vtkRenderWindowInteractor lets the vtkInteractionStyle (and subclasses) do all
the heavy lifting. ParaView did that to some extent (since it has a
vtkPVInteractorStyle), but will was relying on
`vtkPVGenericRenderWindowInteractor`, `vtkPVRenderViewProxy` to propage
interaction/still renders and other things. This has been refactored. ParaView
no longer uses a special vtkRenderWindowInteractor. All logic is handled by
observers on the standard vtkRenderWindowInteractor.

This change was done to make it easier to enable interactivity in `pvpython`.

See also: vtkSMRenderViewProxy::SetupInteractor(). Subclasses of pqView now pass
the interactor created by QVTKWidget to this method to initialize it.

See also: vtkSMViewProxyInteractorHelper.

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
