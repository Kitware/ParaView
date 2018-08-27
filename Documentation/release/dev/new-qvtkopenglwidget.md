#New QVTKOpenGLWidget Implementation

A new implementation of the QVTKOpenGLWidget has been added to VTK and ParaView whom now makes uses of it.

* The main feature of this new widget is the support of quad buffer based stereo mode "Crystal Eyes" which was unsupported since the switch to Qt5.
* This new widget ensures the rendering can only happen with a valid widget (aka ready for rendering)
* This new widget supports HiDpi rendering
* This new widget also improves a few issues of flickering with the old widget.
* The old widget have been kept and renammed QVTKOpenGLNativeWidget and should be used for simple rendering or when the widget is automatically native.
* This new QVTKOpenGLWidget class does not support Qt::WA_NativeWindow flag but the QScrollArea used in pqTransferFunctionWidget force it to be native.
* The old widget is used in pqTransferFunctionWidget
