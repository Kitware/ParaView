## Change behavior of server side command line execution in multi-process case

When running `pvserver` with MPI, the command specified by the CLI option `--timeout-command` now runs only on the first rank.
