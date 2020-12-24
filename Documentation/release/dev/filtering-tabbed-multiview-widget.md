## Filtering in pqTabbedMultiViewWidget

`pqTabbedMultiViewWidget` which is used to show multiple view layouts as tabs
now supports filtering based on annotations specified on the underlying layout
proxies. Checkout the `TabbedMultiViewWidgetFilteringApp.cxx` test to see how this
can be used in custom applications for limiting the widget to only a
subset of layouts.
