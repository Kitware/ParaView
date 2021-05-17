# paraview-node-editor

Integration from https://github.com/JonasLukasczyk/paraview-node-editor

### Overview
This repository contains a node editor plugin for ParaView that makes it possible to conveniently modify filter/view properties (node elements), filter input/output connections (blue edges), as well as the visibility of outputs in certain views (orange edges). The editor is completely compatible with the existing ParaView widgets such as the pipeline browser and the properties panel (one can even use them simultaneously). So far the plugin is self-contained, except for a tiny hotfix of the ParaView source code. I use this editor in my production workflow and I would even go as far as saying the current version is stable. There are some minor issues that result from the fact that some ParaView widgets explicitly call QPaintDevice render calls that are incompatible with the Graphics View framework (https://doc.qt.io/qt-5/qwidget.html#custom-widgets-and-painting). However, these rare cases can be circumvented by using the original widgets if necessary.

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

### Current Limitations
1. Embedded property widgets that show a double input field are only shown when hovered over (integer inputs and even the calculator work). The source of this problem is that the pqDoubleLineEdit class uses some explicit render calls that are incompatible with the QT Qraphics View framework. A hotfix for this issue is provided in step 1 of the installation (see above), which just circumvents this explicit render call.
2. Widgets that can show/hide an interactor in a view are currently not working correctly.
3. Filters that require multiple inputs open upon creation a selection dialog for the inputs, which is no longer necessary in the node editor.
4. If a property opens a separate dialog window (e.g., a file browser or color selector), then this dialog is embedded in the graphics view and not shown as a modal popup window. Some of these dialog windows are not correctly rendered in the graphics view (which is again a result of explicit render calls in the PV source).
