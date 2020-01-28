## Improvements to ParaView LIVE

There have been several improvements to the code that sets up connections
between simulation processes and ParaView (or pvserver) processes when using
LIVE.

* When using ParaView in builtin mode (i.e. without pvserver) as the LIVE
  viewer, you no longer need to punch an SSH tunnel hole for chosen catalyst
  port + 1 (default: 22223). We now use the reuse socket connection between
  Catalyst root node and ParaView for exchange meta-data as well as transferring
  data.

* In configurations with multiple ranks for simulation (Catalyst) and
  ParaView (pvserver), each Catalyst satellite rank connects to every ParaView satellite
  rank using sockets. Previously, the port these connections are made on was
  hard-coded to be a fixed offset from the user-selected live connection port.
  That is no longer the case. Each ParaView satellite process now picks any available
  port thus avoiding port conflicts on shared systems.
