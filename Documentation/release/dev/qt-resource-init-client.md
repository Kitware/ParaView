## Allow pqApplicationComponentsInit in non-static builds.

ParaView needs to call pqApplicationComponentsInit() to initialize QT
resource only for static builds. However, if client apps want to
re-use ParaView QT resource for their own menus/toolbars, they
need to call pqApplicationComponentsInit() always, even for shared
builds. This is now possible.
