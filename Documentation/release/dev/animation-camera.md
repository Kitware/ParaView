## Camera Animation : improve track creation and edition

This adds some options in the `KeyFrame` editors, leading to:
 - more flexible camera modes while keeping previous features.
 - easier camera configuration in interpolation mode

Camera modes are now as follow.

### Follow path
Each `keyframe` is a spline for camera position, and one, independent for camera focus.
An orbit (i.e. a circle) can be configured from the editor (instead of from cue creation),
by default around the selected source from current camera.

### Interpolate cameras
Each `keyframe` is a camera.
The animation interpolate between cameras, either with spline or line depending on the relevant option
in the editor dialog.

Appropriate buttons allow to use / setup current camera position.

### Follow data
A "Reset Camera" is done on each frame, with selected data as active object. No `keyframe` on this mode.


Doing this, we remove options from the camera cue creation:
 - Orbit. Instead, use `Follow path` and the `orbit` button from the editor to setup an orbit on current keyframe.
 - Interpolate camera locations (spline). Use `Interpolate cameras`. It uses spline by default.
 - Interpolate camera locations (linear). Use `Interpolate cameras`, and uncheck `Use spline` option in the key frame editor.
