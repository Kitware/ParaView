## Configure Proxy RegistrationName for pipeline name

When creating a proxy (eg, on loading a file), the pipeline source created used to be named according to the file for readers
or to the name of the proxy for other proxies.
This is not always the best choice, so ParaView now check for `RegistrationName` information string property
in the proxy. If found, the registration name (hence the pipeline name for readers and filters) is the one given by the information property.
Otherwise, it falls back to the previous logic.
