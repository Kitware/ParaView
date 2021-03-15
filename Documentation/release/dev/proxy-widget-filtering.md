## Improvements to pqProxyWidget

`pqProxyWidget` is the widget used to auto-generate several panels based on
proxy definitions. The class supports two types of panel visibilities for
individual widgets for properties on the proxy: **default** and **advanced**.
These needed to match the value set for the `panel_visibility` attribute used
the defining the property in ServerManager XML. `pqProxyWidget` now has API to
make this configurable. One can now use arbitrary text for `panel_visibility`
attribute and then select how that text is interpreted, default or advanced, by
a particular `pqProxyWidget` instance. This makes it possible for proxies to
define properties that are never shown in the **Properties** panel, for example,
but are automatically shown in some other panel such as the **Multiblock
Inspector** without requiring any custom code. For more details, see
`pqProxyWidget::defaultVisibilityLabels` and
`pqProxyWidget::advancedVisibilityLabels`.
