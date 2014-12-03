Implementation Details: Slice View    {#DesignSliceView}
===================================

![Slice View in ParaView](sliceview.png)

[TOC]

This page summarizes the design details of what ParaView calls the **Slice
View** (sometimes called the **Multi-Slice View** since it allows the users to
add multiple slices).

##Classes##

Let's start by getting an overview of the new classes added for supporting this
Slice view.

vtkSMMultiSliceViewProxy is the view-proxy. It's a vtkSMRenderViewProxy
subclass. This makes it possible for the application to treat this as simply a
specialization of the *Render View*, which it indeed is. Thus all toolbars for
setting axis of rotation, camera positions, selection etc. will continue to
work.

pqMultiSliceView is the pqRenderView subclass created by the GUI whenever it
encounters a vtkSMMultiSliceViewProxy. The extra thing that this class does it
manage the UI components to allow the user to interactively place slices in the
view. Use pqMultiSliceAxisWidget, this class updates the *XSlicesValues*,
*YSlicesValues*, and *ZSlicesValues* properties on the vtkSMMultiSliceViewProxy
proxy.

vtkPVMultiSliceView is a VTK-object controlled by the
vtkSMMultiSliceViewProxy. vtkPVMultiSliceView is a indeed a vtkPVRenderView
subclass. It adds API to set/get the slice positions along each of the
coordinate axes. Representations that want to respect the slices specified by
this view can access the view in vtkPVView::REQUEST_UPDATE() pass to get the
slice positions. This pass will indeed be triggered on all data-server ranks
after the slice positions have been changed.

vtkGeometrySliceRepresentation is a vtkGeometryRepresentation subclass which is
used to add a new **Slices** representation type to the standard
*GeometryRepresentation* proxy.
vtkSMMultiSliceViewProxy::GetRepresentationType() will suggest creating of this
extended *GeometryRepresentation*, know as
(*representations*, *CompositeMultiSliceRepresentation*) to the ProxyManager.

*CompositeMultiSliceRepresentation* supports the usual representation-types e.g.
Surface, Wireframe, etc. along with the new *Slices* type. Whenever the user
picks the *Slices* representation-type, the vtkGeometrySliceRepresentation will
be used.

vtkGeometrySliceRepresentation works with vtkPVMultiSliceView to display any
dataset as a set of slices along the coordinate planes. The slice positions are
obtained from vtkPVMultiSliceView in the vtkPVView::REQUEST_UPDATE().

vtkGeometrySliceRepresentation works by replacing the vtkPVGeometryFilter (in
vtkGeometryRepresentation) with an internal vtkPVGeometryFilter subclass that
extracts the slices from the input dataset before passing it to the
vtkPVGeometryFilter implementation. It uses the vtkThreeSliceFilter to apply a
collection of slice filters in one go.

##Supporting Miscellaneous Use-Cases##

Let's look at how we support a bunch of miscellaneous use-cases.

###Choosing *Slices* representation by default###

**Use-Case**: When user shows a new dataset in this Slice view, we want the
default representation-type to be "Slices", and not "Surface" or anything
that vtkSMRepresentationTypeDomain picks.

**Solution (HACK)**: vtkSMMultiSliceViewProxy::CreateDefaultRepresentation()
changes the *Representation* property's value to *Slices*. Further more, to
avoid vtkSMParaViewPipelineController::PostInitializeProxy() from resetting the
property value to domain-defaults (using
vtkSMProperty::ResetToDomainDefaults()), we add an XML hint to the property.
This is done in vtkSMMultiSliceViewProxy::ForceRepresentationType().

###Setting default slice locations###

**Use-Case**: When a new dataset is shown in this view, we want the view to
automatically place slices at the center of the dataset.

**Solution (HACK)**: We set the properties on the view in
vtkSMMultiSliceViewProxy::CreateDefaultRepresentation(). Currently the
properties on the view are changed only when there are no slices specified
already.

Notes
------
This page is generated from *MultiSliceView.md*.
