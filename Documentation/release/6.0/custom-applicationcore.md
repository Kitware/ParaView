## Custom ApplicationCore class for ParaView based application

ParaView based application already can specify a `MAIN_WINDOW_CLASS`
in `paraview_client_add` CMake method. This is in fact the main point of a those kind of app.

You can now also specify a `APPLICATION_CORE_CLASS`, that should subclass
the `pqPVApplicationCore` default class.

It is useful to control core features at creation, before the end of the initialization.
An example is using or not the app version in the settings filename.
(see pqApplicationCore::useVersionedSettings)
