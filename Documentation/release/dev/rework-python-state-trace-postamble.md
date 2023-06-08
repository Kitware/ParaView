## Rework Python trace and state postamble

When saving a Python trace or a Python state,
a postamble is added to help users uses this python script.
It has been improved as a comment with multiple example
usecases.

Critically, it replace the previous Python state postamble that
actually called `SaveExtracts` when the script was run from pvpython,
which is not a behavior that is universally wanted.
