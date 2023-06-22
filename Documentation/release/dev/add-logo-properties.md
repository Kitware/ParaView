## Add vtkLogoSourceRepresentation properties

You can now control how your logo's border is rendered.
* You can choose when to render a border (never, always, on hover).
* You can set border thickness and color.

You can now also set a scale that defines height relatively to screen size.
If set to 1, your logo's height will match your render view's height. The
width will be scaled accordingly to preserve the image ratio.

To enable this scaling, you have to first set the interactive scaling to Off.
When interactive scaling is On (by default), the behavior remains unchanged,
which means you can scale it only by grabbing the corners of your logo.

You will now also have the border fitting the image (it's a bug fix).
