## Fast preselection

A fast preselection option is available in the render view settings.\
When enabled, the preselection reuses visible geometry to display preselection.\
This is a lot faster than interactive selection extraction, especially for big datasets.\
The drawback is that preselection might behaves slightly differently with translucent geometries (only visible faces of the cells are selected) and high order cells (all subdivisions are visible).
