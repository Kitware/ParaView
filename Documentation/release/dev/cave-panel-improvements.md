## CAVEInteraction panel improvements

The CAVEInteraction panel now ensures uniqueness of interactor style names before they are added to the set of active styles. This fixes a bug where adding multiple interactor styles with the same combobox values resulted in those styles having the same name, each time causing replacement of the existing style with that name.

Also, you can now simply and quickly reset any navigation performed by your interactor styles, using the new button in the CAVEInteraction panel which resets the Navigation Matrix to the identity matrix.

And finally, you will no longer see the misleading camera orientation axes on the CAVE displays when they are in off-axis mode (the default). Those widgets are tied to desktop camera, and when pvserver displays are in off-axis mode, the desktop camera has no effect.
