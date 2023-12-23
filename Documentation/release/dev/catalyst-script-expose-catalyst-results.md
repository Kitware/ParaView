## Allow providing custom `catalyst_results` in catalyst scripts.

Users can now write custom `def catalyst_results(info)` functions in their
catalyst script which will be executed when the simulation calls
`catalyst_results`. The conduit node passed as input to the call from the
simulation side is available via `info.catalyst_params`.
