Major API Changes             {#MajorAPIChanges}
=================

This page documents major API/design changes between different versions since we
started tracking these (starting after version 4.2).

Changes in 4.3
--------------

###1. Replaced `pqCurrentTimeToolbar` with `pqAnimationTimeWidget`###

`pqCurrentTimeToolbar` was a `QToolbar` subclass designed to be used in an
application to allow the users to view/change the animation time. We
reimplemented this class as `pqAnimationTimeWidget` making it a `QWidget`
instead of `QToolbar` to make it possible to use that in other widgets e.g.
`pqTimeInspectorWidget`. `pqAnimationTimeToolbar` was also changed to use
`pqAnimationTimeWidget` instead of `pqCurrentTimeToolbar`.

Besides just being a cleaner implementation (since it uses `pqPropertyLinks`
based linking between ServerManager and UI), `pqAnimationTimeWidget` also allows
applications to change the animation *play mode*, if desired. The rest of the
behavior of this class is similar to `pqCurrentTimeToolbar` and hence should not
affect applications (besides need to update any tests since the widget names
have now changed).
