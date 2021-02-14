## Decorator to show/hide or enable/disable properties based on session type

Certain properties on a filter may not be relevant in all types of connections
supported by ParaView and hence one may want to hide (or disable) such
properties except when ParaView is in support session. This is now supported
using `pqSessionTypeDecorator`.
