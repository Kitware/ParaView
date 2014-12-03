Implementation Details: Slice View    {#DesignSliceView}
===================================

This page summarizes the design details of what ParaView calls the **Slice
View** (sometimes called the **Multi-Slice View** since it allows the users to
add multiple slices).

##Classes##

###`vtkSMMultiSliceViewProxy`###
`vtkSMMultiSliceViewProxy` is the view-proxy. It's a `vtkSMRenderViewProxy`
subclass. This makes it possible for the application to treat this as simply a
specialization of the *Render View*, which it indeed is. Thus all toolbars for
setting axis of rotation, camera positions, selection etc. will continue to
work.

###`pqMultiSliceView`###
`pqMultiSliceView` is the `pqRenderView` subclass created by the GUI whenever it
encounters a `vtkSMMultiSliceViewProxy`. The extra thing that this class does it
manage the UI components to allow the user to interactively place slices in the
view. Use `pqMultiSliceAxisWidget`, this class updates the *XSlicesValues*,
*YSlicesValues*, and *ZSlicesValues* properties on the vtkSMMultiSliceViewProxy
proxy.

###`vtkPVMultiSliceView`###



`vtkPVMultiSliceView` | The vtkPVRenderView subclass for this view, the VTK object







Notes
------
This page is generated from *MultiSliceView.md*.
