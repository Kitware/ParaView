## Defer default OSPRay materials loading until pathtracing activation

Connecting to a Paraview server with raytracing enabled used to be very slow, because default OSPRay materials were sent from the client to the server on session startup. Now this long loading time is deferred until OSPRay pathtracing is actually used.
