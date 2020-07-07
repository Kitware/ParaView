# Preview mode and text scaling

Currently a single scale factor is when saving rendering results using a screenshot.

This MR introduces a separate scale factor which is passed on the pqQVTKWidget. This ensures
that VTK always renders at the preview resolution despite the fact that
the Qt widget is at a different resolution. This makes it possible to
show 2D text (and other annotations) faithfully in preview mode.
