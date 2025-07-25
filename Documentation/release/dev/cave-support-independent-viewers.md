## CAVE: Support for independent viewers

ParaView running in CAVEs now supports multiple simultaneous head-tracked users. Via the
`.pvx` configuration file, you can specify a `ViewerId` attribute for each `Machine`
element. Via new render view proxy properties, you can specify eye transform matrices and
eye separation values for any number of users. You can use the CAVE configuration panel
to adjust the eye separations for each user.

### Configuration using .pvx

The simplest way to add multiple users is just to add a `ViewerId` attribute to each `Machine`
element. Viewer ids must include only and all values between `0` and the number of viewers
minus `1`. You can associate a single viewer id with any number of `Machine` elements.

You can also configure unique eye separations for each viewer by adding an `IndependentViewers`
section to your `.pvx` file, see `Examples/CAVE/twoViewers.pvx` for an example.

### Configuring eye separation using the UI

The `Track` interactor style proxy now works with any number of independent viewers and is also
easier to use. When you select the `Track` style interactor style proxy from the dropdown menu,
you now only need to select the correct RenderView proxy, as the property is no longer
configurable. When you add the `Track` interactor style, the plugin queries the server for the
number of independent viewers and dynamically creates named `Tracker` roles for the correct
number of viewers, at which point you can map your VRPN/VRUI events to those roles using the
familiar dialog.

Also, when you select the `Track` style proxy, you should see a new `EyeSeparation` property,
allowing you to adjust the default (i.e. from `.pvx` or state file) eye separations for each
viewer.
