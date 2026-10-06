# Custom menus and toolbars

This example shows how plugins can add menus and toolbars to ParaView to expose
analysis tools, i.e. sources, filters and Python scripts, at locations of the
developer's choosing. Neither plugin requires compiling: load them using
`Tools > Manage Plugins > Load New` or make them load automatically, e.g. using
the `PV_PLUGIN_PATH` environment variable.

Both plugins add items to the same **Analysis Tools** menu and toolbar.

* `AnalysisTools.xml` is an XML-only plugin. It defines **Surface With
  Normals**, a pipeline of standard filters exposed as a single filter, and adds
  a source, standard filters, the new filter and inline Python scripts to the
  menu and toolbar, including a sub-menu.

* `AnalysisToolsPython.py` is a Python plugin. It uses the `smmenu` decorators
  from `paraview.util.vtkAlgorithm` to add **Normalized Height**, a filter
  implemented in Python, and Python functions to the menu and toolbar.

## Categories in the menu bar

Menus are categories of the Filters menu, described using a `ParaViewFilters`
element, that are shown in the menu bar:

```xml
<ServerManagerConfiguration>
  <ParaViewFilters>
    <Category name="My Tools" show_in_menubar="1" show_in_toolbar="1" preserve_order="1">
      <Proxy group="sources" name="SphereSource" />
      <Proxy group="filters" name="Cut" />
      <Separator />
      <Script name="SayHello" label="Say Hello" tooltip="Prints a greeting.">
        print("Hello!")
      </Script>
      <Category name="More">
        ...
      </Category>
    </Category>
  </ParaViewFilters>
</ServerManagerConfiguration>
```

* `show_in_menubar="1"`, for top-level categories, shows the category as a menu
  of the menu bar instead of a sub-menu of the Filters menu.
* `show_in_toolbar="1"` adds a toolbar with the same items. For categories
  shown in the menu bar, sub-categories are shown as drop-down buttons.
* `preserve_order="1"` keeps the items in the given order instead of sorting
  them alphabetically, which is needed for separators.

Categories with the same name are shared, so several plugins can add items to
them. Items are:

* `Proxy`: a filter (`group="filters"`), applied to the selected pipeline
  objects, or a source (`group="sources"`), with an optional `icon`.
* `Script`: Python code, inline or using a `filename` attribute, with optional
  `icon` and `tooltip`. The `name` identifies the script, e.g. when users
  customize the categories, so it should be unique, e.g. prefixed by the name
  of the plugin. Use a `<![CDATA[ ... ]]>` section for code containing `<` or
  `&`.
* `Separator`.

`icon` can be a Qt resource path, e.g. `:/pqWidgets/Icons/pqCalculator.svg`, or
a file path. Users can further customize the menus and toolbars using
`Tools > Configure Filter Categories`.

## `smmenu` decorators

```python
from paraview.util.vtkAlgorithm import smmenu, smproxy, ...

@smmenu.proxy(menu="My Tools", toolbar=True)
@smproxy.filter(label="My Filter")
...
class MyFilter(VTKPythonAlgorithmBase):
    ...

@smmenu.action(menu="My Tools/More", label="Say Hello", toolbar=True)
def say_hello():
    """Prints a greeting."""
    print("Hello!")
```

`smmenu.action` adds a module-level function, called when the item is
triggered. `smmenu.proxy` adds a filter or source defined in the plugin. Use `/`
in `menu` for sub-menus.
