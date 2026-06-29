## Removed View Property **WindowResizeNonInteractiveRenderDelay**

The property **WindowResizeNonInteractiveRenderDelay**, which set the time in seconds before doing a full-resolution render after the last WindowResize event has been removed. Now, the view is doing full-resolution render when resizing a window.

## Developer Notes

The public method **SetResizingWindow** in **vtkSMRenderViewProxy** has been deprecated.
The protected method **SetResizingWindow** in **vtkSMViewProxyInteractorHelper** has been removed.
The protected variable **EndWindowResizeTimerId** in **vtkSMViewProxyInteractorHelper** has been removed.
