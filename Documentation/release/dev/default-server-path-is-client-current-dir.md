## Default server path is client current working directory

Previously, the file dialog opened to the current working directory of the server
when opening the file dialog for the first time, which was not very useful.

Now, ParaView will try to open the client current working directory as a default path
for the server when opening the file dialog for the first time. If it does not exists,
it will fall back on the previous behavior.

This is very useful for user using shared file systems.
