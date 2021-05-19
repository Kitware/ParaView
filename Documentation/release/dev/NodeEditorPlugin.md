## Node editor plugin

It is now possible to visualize the pipeline as a node graph like other popular software like Blender.
This is embedded in the NodeEditor plugin and can be compiled by activating the `PARAVIEW_PLUGIN_ENABLE_NodeEditor` cmake option when compiling ParaView.

The users control are :

  * Filters/Views are selected by double-clicking their corresponding node labels (hold CTRL to select multiple filters).
  * Output ports are selected by double-clicking their corresponding port labels (hold CTRL to select multiple output ports).
  * Nodes are collapsed/expanded by right-clicking node labels.
  * Selected output ports are set as the input of another filter by double-clicking the corresponding input port label.
  * To remove all input connections CTRL+double-click on an input port.
  * To toggle the visibility of an output port in the current active view SHIFT+left-click the corresponding output port (CTRL+SHIFT+left-click shows the output port exclusively)
