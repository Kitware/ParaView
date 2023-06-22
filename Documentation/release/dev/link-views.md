# Link Views

ParaView is able to link objects, so modifications made on one
are forwarded to the other. However, we observe some limitations:
 - ProxyLink link any kind of proxies but does not trigger a rendering
 when used with views.
 - CameraLink has this rendering capabilities but it links only camera properties
 of render view.

So we introduce the ViewLink, that bridge this gap: properties of
any kind of view (not only the classical render view) are
linked AND the views are rendered automatically on changes.
The ViewLink exclude the camera properties. A CameraLink is still needed for that.

The python interface in `simple.py` module is also improved.
New link creation API were added. Creating a link returns its name,
so link can be easily retrieved and removed.
