## Add autocomplete to Python calculator

Add autocomplete capabilities to the the Python calculator expression line editor. This is done using a new `StringVectorProperty` hint widget attribute: `autocomplete="python-calc"`.

2 new interfaces have been created, to wrap autocomplete widget capabilities and Python autocomplete, already used for the Python shell:
 - `pqWidgetCompleter` is a general completion interface.
 - `pqPythonCompleter` provides specific mechanisms for Python completers.
