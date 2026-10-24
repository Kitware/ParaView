## Save State and Screenshot fixes

The SaveStateAndScreenshot plugin no longer disables the _Save State and Screenshot_ button when you change the active view. Previously, you had to click _Configure Save_ again every time a different view became active. Now the saved settings follow the active view, including views you have just created. If the image covers the same layout (**Save All Views** is on and the new view is in the same layout), all your settings, including **Image Resolution**, are kept. Otherwise, **Image Resolution** is set to the size of the new view or layout when you save, and the other settings are kept. The button is still disabled when there is no active view, or when the active view is not in a layout.

The plugin also has these fixes:

- File names that contain dots, such as `run.v2.pvsm`, are no longer cut at the first dot.
- Only the `-yyyyMMdd-hhmmss` timestamp that the plugin adds is removed from the chosen name, so a name such as `case-1-2-final` is kept as it is.
- Canceling the file dialog in _Configure Save_ now cancels the whole configuration.
- Running _Configure Save_ again keeps the PNG options, such as compression level and **Embed ParaView State**.
- Disconnecting from the server or resetting the session clears the saved screenshot settings, so you need to run _Configure Save_ again.
