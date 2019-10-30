# Loading of plugins

In earlier versions, plugins could be loaded before or after the main window in
the application was created depending on whether the plugin was loaded using the
**Load Plugin** dialog or was being auto-loaded. With recent changes, we are now
assuring that plugins are always loaded **after** the main window is created.
This enbles the plugin developers can assume existence of a
main window irrespective of whether the plugin was auto-loaded or loaded after
the application has started.
