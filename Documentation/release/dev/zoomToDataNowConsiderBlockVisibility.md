# Zoom to data actions now consider the visibility of the source and blocks

The Zoom To Data actions (Zoom To Data and Zoom Closest To Data) now consider the visibility of the source and its blocks,
If only some blocks of the source are visible, then the Zoom To Data zoom only on the bounding box of the **visible** blocks,
only in builtin and remote rendering mode.

Also disable the Zoom To button when the active source is not visible.
