Server Connection Improvements
------------------------------

There has been multiple improvement to the server connection.

1. When creating a new server, a default name is provided
2. Fix an issue with SSH server parsing
3. Add the concept of server name to pqServerResource and rely on it when available (see below)
4. Remove the concept a session scheme and session server in pqServerResource (see below)

pqServerResource can now contains a serverName, resulting in the following URI:

```
<connection-scheme>:[//<server-details>]/<path-to-data-file>[#serverName]
```

When available, this serverName is now used to list and open recent files and states, favorites folder and
differentiate more generally between servers of the same schemehostports URI.
When not available, behavior stays as before.

pqServerResource concept of `session` scheme has been removed and related methods have been deprecated.
This causes a small retro compatibility issue where `session` recent state file will not be detected.
Resetting the settings (`Edit -> Reset to default settings`) is a simple fix to that.
Also application code relying on this `session` scheme to parse pqServerResource will not be able to do so anymore.
The, already present, PARAVIEW_STATE data should be used instead.
Finally, pqServerConfiguration::actualResource() has been unconsted.
