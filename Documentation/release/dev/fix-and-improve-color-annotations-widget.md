## Fix and improve color annotations widget

Fix:
- Global modifications in `pqColorAnnotationsWidget` (visibility and opacity values set in the header) now apply on filtered items only.

Improvements :
 - The filtering / sorting section of the the context menu (see `pqTreeViewSelectionHelper`) now appears only for filterable / sortable columns.
 - The opacity values of highlited items are set the same way the visibility states are, that is using the context menu.
 - As the opacity calues of highlited items are set via the context menu, the global opacity dialog of the `pqColorAnnotationsWidget` appearing
   when double-clicking on the table header doesn't fulfill this role anymore.
