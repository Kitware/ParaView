# Generating warning/information/error logs

ParaView now supports generating logs with varying levels of verbosity to help
debug and track down performance bottlenecks.

When logging messages, they are logged at varying verbosity levels. To make
ParaView show log messages at all levels lower than a specific value in the
range [-2, 9], use the `-v=[level]` command line option, where -2 is used for
errors, -1 for warnings, 0 for info, and 9 for trace.

One can request log messages to be logged to files instead using
`-l=<filename>,<verbosity>` command line option. Multiple `-l` options can be
provided to generate multiple log files with different verbosities, if needed.

When logging messages, ParaView logs then using categories with ability for user
to elevate the logging level for any specific category using environment
variables. By elevating the level for a specific category, e.g.
`PARAVIEW_LOG_RENDERING_VERBOSITY` to 0 or INFO, one can start seeing those
messages on the terminal by default without having to change the stderr verbosity thus
causing the terminal to be clobbered with all other log messages not related to
the chosen category.
