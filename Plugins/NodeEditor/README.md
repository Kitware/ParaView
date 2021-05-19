# paraview-node-editor

Integration from https://github.com/JonasLukasczyk/paraview-node-editor

### Overview
This plugin contains a node editor for ParaView that makes it possible to conveniently modify filter/view properties (node elements), filter input/output connections (blue edges), as well as the visibility of outputs in certain views (orange edges). The editor is completely compatible with the existing ParaView widgets such as the pipeline browser and the properties panel (one can even use them simultaneously). So far the plugin is self-contained.

The plugin uses GraphViz for computing an automatic layout of the graph.
Graphviz is an optional dependency so if no library is found then the auto layout feature will be disabled.

### Current Features
1. Automatically detects the creation/modification/destruction of source/filter/view proxies and manages nodes accordingly.
2. Automatically detects the creation/destruction of connections between ports and manages edges accordingly.
3. Every node exposes all properties of the corresponding proxy via the pqProxiesWidget class.
4. Property values are synchronized within other widgets, such as the ones shown in the properties panel.
5. Proxy selection is synchronized with the pipeline browser.
6. Works with state files and python tracing.

### User Manual
* Filters/Views are selected by double-clicking their corresponding node labels (hold CTRL to select multiple filters).
* Output ports are selected by double-clicking their corresponding port labels (hold CTRL to select multiple output ports).
* Nodes are collapsed/expanded by right-clicking node labels.
* Selected output ports are set as the input of another filter by double-clicking the corresponding input port label.
* To remove all input connections CTRL+double-click on an input port.
* To toggle the visibility of an output port in the current active view SHIFT+left-click the corresponding output port (CTRL+SHIFT+left-click shows the output port exclusively)

### Limitations

See current limitations here : https://gitlab.kitware.com/paraview/paraview/-/issues/20726
