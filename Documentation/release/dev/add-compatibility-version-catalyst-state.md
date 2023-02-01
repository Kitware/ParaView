## Add compatibility version for Catalyst state

The Catalyst state now also includes the compatibility version in the beginning of the state file.
Additionally, if `paraview.compatibility` is set and a deprecated proxy, property, or property value needs
to be changed, a warning will be printed.
