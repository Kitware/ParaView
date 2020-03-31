## Log Viewer window

We added a new Log Viewer window to ParaView. This window enables creation of individual log viewers for ParaView processes in the possible configurations in which ParaView can be run (client with built-in server, client with remote server, client with remote data server and remote render server). For configurations where server components run as MPI jobs, a log created in this viewer is restricted to one rank in the parallel run.

An arbitrary number of log viewers can be created and destroyed. The logging level that controls the verbosity of the log can be changed dynamically on each ParaView process independently.

In addition, different ParaView logging categories (PARAVIEW_LOG_DATA_MOVEMENT_VERBOSITY, PARAVIEW_LOG_RENDERING_VERBOSITY, PARAVIEW_LOG_APPLICATION_VERBOSITY, PARAVIEW_LOG_PIPELINE_VERBOSITY, and PARAVIEW_LOG_PLUGIN_VERBOSITY) can be promoted to more severe log levels to show only log messages of interest.
