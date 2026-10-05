## Catalyst pipelines that throw are disabled instead of stopping Catalyst

When a Catalyst pipeline throws an exception while initializing, executing or
producing results, ParaView Catalyst now logs the error and disables that
pipeline, and the other pipelines keep running on later steps. Previously the
exception escaped `catalyst_execute()`, which usually ended the simulation, or,
under Catalyst's asynchronous execution, left every later step failing. Any
other exception in a Catalyst call is reported as an error status instead of
crossing the C interface.
