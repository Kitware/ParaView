## Python initialization during import

To make creation of various proxies easier, ParaView defines classes for each
known proxy type. These class types were immediately defined when `paraview.simple`
was imported or a connection was initialized. Creation of these class types is
now deferred until they are needed. This helps speed up ParaView Python
initialization.

This change should be largely transparent to users except for those who were
directly accessing proxy types from the `paraview.servermanager` as follows:

    # will no longer work
    cls = servermanager.sources.__dict__[name]

    # replace as follows (works in previous versions too)
    cls = getattr(servermanager.sources, name)
