This application does not rely on a WAMP server, just custom session manager
configuration that will use the provided launcher.sh script.

The launcher.sh is provided to illustrate how a parallel visualization can be
achieved using to extra launching options.
But customization should happend for a real deployment. In fact, SSH should
be involved to trigger the visualization process on remote computers
and not on the WebServer like it is currently done in the script.

#######################################
# Jetty Session Manager configuration #
#######################################

pw.launcher.cmd=./launcher.sh PORT CLIENT RESOURCES FILE SECRET
pw.launcher.cmd.run.dir=/path-that-contains-launcher-script
pw.launcher.cmd.map=PORT:getPort|SECRET:secret|CLIENT:client|FILE:file|RESOURCES:resources
