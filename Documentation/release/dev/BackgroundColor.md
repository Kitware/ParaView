Render View Background Color
----------------------------

Render View (and related views) now have a property
**UseColorPaletteForBackground** which indicates if the view-specific background
overrides should be used or the view should simply use the global color palette
to determine how the background is rendered. Previously, it was confusing to
know which views were using the global color palette and which ones were using
user overridden values for background color.

Also, instead of having separate boolean properties that are largely mutually
exclusive, the view now adds a `BackgroundColorMode` property that lets the user
choose how the background should be rendered.
Supported values are `"Single Color"`, `"Gradient"`, `"Texture"` and `"Skybox"`.

This change impacts the Python API. `UseGradientBackground`, `UseTexturedBackground`
and `UseSkyboxBackground` properties have been replaced by `BackgroundColorMode`
which is now an enumeration which lets user choose how to render the background.
Also, unless compatibility <= 5.9 was specified, view specific background color
changes will get ignored unless `UseColorPaletteForBackground` property is not
set to `0` explicitly.
