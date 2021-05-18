# View Property WindowResizeNonInteractiveRenderDelay

A new double property, 'WindowResizeNonInteractiveRenderDelay', on the view has been added
that lets you set the time in seconds before doing a full-resolution render after the last WindowResize
event. This allows the view to render interactively when resizing a window, and can improve the
performance when using large data.
