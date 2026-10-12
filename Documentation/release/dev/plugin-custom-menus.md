## Plugins can add menus and toolbars for their tools

Categories of the Filters menu can now be shown as menus of the menu bar, using the `show_in_menubar` attribute, and contain sources, Python scripts and separators in addition to filters. This lets plugins, including XML-only and Python plugins that do not require compiling, gather their analysis tools in menus and toolbars of their own. Several plugins can add items to the same menu and toolbar, and a category with the title of an existing menu, such as *View*, adds its items to this menu. Toolbars of categories shown in the menu bar show their sub-categories as drop-down buttons. *Tools > Configure Filter Categories* can also show top-level categories in the menu bar: its tree now has a *Filters* item containing the categories of the Filters menu, the ones shown in the menu bar being beside it.

Python plugins can use the new `smmenu` decorators from `paraview.util.vtkAlgorithm` to add functions, filters and sources defined by the plugin to menus and toolbars. See the *Adding Menus and Toolbars for Categories* section of the plugin documentation and `Examples/Plugins/CustomMenus` for complete examples.

### Developer notes

`pqCategoryMenuBarBehavior` shows categories in the menu bar, if the main window has one, in which case `pqProxyGroupMenuManager::showCategoriesInMenuBar()` is true and these categories are not shown in the menu of the manager. Items of a `pqProxyCategory`, described by `pqProxyInfo`, can now be Python scripts and separators in addition to proxies.
