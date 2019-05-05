# Improvements to parallel rendering support

This release includes changes to the parallel rendering code to simplify the
logic for developers and maintainers along with addressing some long standing
issues with tile displays and CAVE rendering especially when dealing with
split views.

Developers developing new view types should note that the view API has been
simplified considerably. Earlier views needed to handle the case where all views
could be sharing the same render window on server processes. That is no longer
the case. Each view now gets a complete render window and has control over the
renderers added to that window. Server processes still have a share rendering
window which is used to display the combined results from multiple views to the
user via vtkPVProcessWindow, but vtkPVView subclasses don't have to handle it.
`vtkViewLayout` manages all the collecting of rendering results from each view
and posting them to a user viewable window, if and when needed.

In CAVE mode, we now handle split views more consistently by ensuring only one
of the views is presented in the CAVE. Currently, it's the most recently
rendered view.

Additionally, more logging entries were added that are logged under the
`PARAVIEW_LOG_RENDERING_VERBOSITY` category that should diagnose parallel
rendering issues.
