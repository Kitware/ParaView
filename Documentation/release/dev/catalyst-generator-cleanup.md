# promote coprocessing plugin's writers and generator to catalyst menu

We removed the CoProcessingScriptGenerator plugin and promoted
its functions to mainline ParaView. As before they depend on
Python and Qt, but otherwise they are now always available.
With this we removed the plugin's two menus and moved their
contents under the Catalyst menu.
