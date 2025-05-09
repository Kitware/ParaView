## Provide additional paths to python libraries in ParaViewCatalyst Blueprint.

The `initialize` protocol of ParaView-Catalyst Blueprint has now a new optional
entry `catalyst/python_path`.

`catalyst/python_path` allows to add a list of
paths to be added in the `PYTHONPATH` on the Python interpreter that executes
the catalyst python script.
`catalyst/python_path` should be given as a single string.
Multiple paths should be separated with the system path separator i.e. : for
Unix and ; for Windows.
