## Catalyst 2.0: direct access to conduit during execution

The input parameters of `catalyst_execute` are now accessible as a read-only
conduit node inside a catalyst script.  You can now perform any kind of
analysis directly on the data passed by the simulation.
