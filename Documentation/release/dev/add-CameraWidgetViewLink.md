## Add a camera widget view link

You can now link two render views to enable one to control the camera of the other
through a new widget. You can create this new link in the `Manage Link` window
(under the `Tools` tab). The first render view you should select is the one in
which you want the camera widget to appear, and the second one should be the render
view which you want to control the camera. You can also create it via the
`Add Camera Link` (in `Tools` too) or the `Link Camera` (when right clicking on a
render view). Note that it won't link cameras of render view when doing so, but
only allow you to control a camera in another render view.

The camera widget will let you control the camera position, target, orientation and
view angle via various handles. Moving the camera from the second view will
automatically update the camera widget, and vice versa.

![Illustration of using the camera widget view link](add-CameraWidgetViewLink.gif)
