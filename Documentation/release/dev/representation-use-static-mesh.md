## Faster time to first render with static meshes

ParaView now uses a cache to save time when computing surfaces to display.
If the surface mesh is unchanged since the last time it was extracted and caching is enabled,
the extraction step simply uses the cached result, saving significant work in the steps leading up to the first render after a change is made.

This is not available in distributed execution for now.
