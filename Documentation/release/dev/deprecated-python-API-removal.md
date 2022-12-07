## Deprecated python API removal

Several deprecated python API that have been around for a few years are now gone.
This include :

- `paraview.simple.CreateScalarBar()`, replaced by `paraview.simple.GetScalarBar()`.
- `paraview.simple.WriteAnimation()`, replaced by `paraview.simple.SaveAnimation()`.
- `paraview.simple.WriteImage()`, replaced by `paraview.simple.SaveScreenshot()`.
- `paraview.simple.ImportCinema()`, no longer supported since 5.9.
- `paraview.compatibility.GetVersion().GetVersion()`, because of float comparison issue.

Other deprecated behaviors have been removed :

- `paraview.compatibility.GetVersion()` cannot be compared with float values anymore.
- In `paraview.simple.CreateView()`, removed support for positional argument `detachedFromLayout`.
- In `paraview.simple.SaveScreenshot()`, removed support for optional argument `ImageQuality`.
- In `paraview.simple.SaveAnimation()`, removed support for optional argument `DisconnectAndSave` and `ImageQuality`.
