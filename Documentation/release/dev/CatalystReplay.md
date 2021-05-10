## Add Conduit Node IO and Catalyst Replay Executable ##

To assist in debugging in-situ pipelines, Catalyst now
supports `conduit_node` I/O. The `params` argument to each invocation of
`catalyst_initialize`, `catalyst_execute`, and `catalyst_finalize` can be
written to disk. These can later be read back in using the `catalyst_replay`
executable. See the corresponding documentation:
https://catalyst-in-situ.readthedocs.io/en/latest/catalyst_replay.html
