ParaView now supports plugins that add to or replace the default context menu,
via the pqContextMenuInterface class. The default ParaView context menu code
has been moved out of pqPipelineContextMenuBehavior into pqDefaultContextMenu.

Context menu interface objects have a priority; when preparing a menu in
response to user action, the objects are sorted by descending priority.
This allows plugins to place menu items relative to others (such as the
default menu) as well as preempt all interfaces with lower priority by
indicating (with their return value) that the behavior should stop iterating
over context-menu interfaces.

There is an example in `Examples/Plugins/ContextMenu` and documentation
in `Utilities/Doxygen/pages/PluginHowto.md`.
