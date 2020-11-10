ParaView-Catalyst Blueprint {#ParaViewCatalystBlueprint}
===========================

**This page is applicable to ParaView Catalyst implementation introduced in
ParaView 5.9.**

-----

## Background

Starting with ParaView 5.9, ParaView distribution provides an implementation of
the [Catalyst API](https://catalyst-in-situ.readthedocs.io/). This
implementation is now referred to as **ParaView-Catalyst**. Thus, it an
implementation of the Catalyst In Situ API that uses ParaView for data
processing and rendering.

The Catalyst in situ API comprises for 3 main function calls that are used to
pass data and control over to the Catalyst implementation from a computational
simulation codes: `catalyst_initialize`, `catalyst_execute`, and
`catalyst_finalize`. Each of these functions is passed a
[Conduit Node](https://llnl-conduit.readthedocs.io/en/latest/tutorial_cpp_basics.html)
object. Conduit Node provides a flexible mechanism for describing hierarchical
in-core scientific data. Since Conduit Node is simply a light-weight container,
we need to define the conventions used to communicate relevant data in each of
the Catalyst API calls. Such conventions are collectively referred to as the
Blueprint. Each Catalyst API implementation can develop its own blueprint.
ParaView-Catalyst, the Catalyst API implementation provided by ParaView, also
defines its blueprint called the **ParaView-Catalyst Blueprint**. This page
documents this blueprint.

## Protocol

A blueprint often includes several protocols, each defining the conventions for
a specific use-case. Since there are three main Catalyst API functions, the
blueprint currently defines three protocols, one for each of the Catalyst
API functions.

* **protocol: 'initialize'**: defines the options accepted by
  `catalyst_initialize`; these include things like ParaView Python scripts to
  load.

* **protocol: 'execute'**: defines the protocol for `catalyst_execute` and
  includes information about Catalyst channels i.e. ports on which data is made
  available to in situ processing as well as the actual data from the
  simulation.

* **protocol: 'finalize'**: defines the protocol for `catalyst_finalize`;
  currently, this is empty.


In each of the Catalyst API calls, ParaView looks for a top-level node named
'catalyst'. The expected children vary based on the protocol described in the
following sub-sections.

These top-level protocols use other internal protocols e.g. 'channel'.

### protocol: 'initialize'

Currently, 'initialize' protocol defines how to pass scripts to load for
analysis.

* catalyst/scripts: (optional) if present must either be a 'list' or 'object'
  node with child nodes that provides paths to the Python scripts to load for
  in situ analysis.

Additionally, one can provide a list of pre-compiled pipelines to use.

* catalyst/pipelines: (optional) if present must be a 'list' or 'object' node
  with child nodes that are objects that provide type and parameters for
  hard-coded pipelines to use. Each object must be in accordance to the
  protocol: 'pipeline'.

In MPI-enabled builds, ParaView is by default initialized to use `MPI_COMM_WORLD`
as the global communicator. A specific MPI communicator can be provided as
follows:

* catalyst/mpi\_comm: (optional) if present, must be an integer representing the
Fortran handle for the MPI communicator to use. The Fortran handle can be
obtained from `MPI_Comm` using `MPI_Comm_c2f()`.

### protocol: 'execute'

Defines now to communicate data during each time-iteration.

**time/timestep/cycle**: this defines temporal information about the current
invocation.

* catalyst/state/timestep: (optional) integral value for current timestep. if not
  specified, 'catalyst/cycle' is used.
* catalyst/state/cycle: (optional) integral value for current cycle index, if not
  specified, 0 is assumed.
* catalyst/state/time: (optional) float64 value for current time, if not specified,
  0.0 is assumed.

**channels**: channels are used to communicate simulation data. The **channels**
node can have one or more children, each corresponding to a named channel. A
channel represents a data-source in the analysis pipeline that is linked to the
data being produced by the simulation code.

* catalyst/channels/[channel-name]: (protocol: 'channel'): (optional) if present
  represents a named channel with the name 'channel-name'. The node must be an
  'object' node satisfying the 'channel' protocol.

The 'channel' protocol is as follows:

* channel/type: (required) a string representing the channel type. Currently,
  the only supported value is "mesh". It is used to indicate that this channel
  is specified in accordance to the
  [Conduit Mesh](https://llnl-conduit.readthedocs.io/en/latest/blueprint_mesh.html#) protocol.

* channel/data: (required) an object node used to communicate the simulation
  data on this channel. This node must match the protocol requirements
  identified by the 'channel/type'.

### protocol: 'finalize'

Currently, this is empty.

### protocol: 'pipeline'

Defines type and parameters for a hard-coded pipeline.

* type: (required) a string identifying the type of the pipeline. Currently
  supported value is "io".

When 'type' is 'io', the following attributes are supported.

* filename: (required) a string representing a filename. `%ts` may be used to
  replace with timestep or `%t` may be used to replace with time value. You can
  use `printf` style format specifiers e.g. `%03t` etc. The filename may be used
  to determine which supported writer to use for saving the data.

* channel: (required) a string identifying the channel by its name.
