## Add a command-line option to choose pvserver socket bind address

Add the `--bind-address` command-line option to `pvserver`.
This option allows changing the address bound to the server socket,
so that it is not necessarily exposed on all interfaces using `INADDR_ANY` (0.0.0.0).
