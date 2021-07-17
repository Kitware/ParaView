## ParaView Node Editor

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

* Filters label :
    - Left click : select filter as active source
    - Left click + CTRL : add filter to the current selection
    - Middle click : Delete the filter
    - Right click : toggle between invisible, normal and invisible mode for the filter properties
* Filters input ports :
    - Left click + CTRL : set all selected output ports as input. If only one input is accepted it will use the last selected object as input.
    - Middle click : Clear all incoming connections for this port
* Filters output ports :
    - Left click: set output port as active selection
    - Left click + CTRL: add output port to current selection
    - Left click + SHIFT: toggle visibility in active view
    - Left click + SHIFT + CTRL: hide all but this port in active view
* View label :
    - Left click : select as active view
    - Right click : increment verbosity
* Drag and drop from a port to an other to create links between 2 filters. This is not supported between a view node and a filter yet.

### Limitations

See current limitations here : https://gitlab.kitware.com/paraview/paraview/-/issues/20726
