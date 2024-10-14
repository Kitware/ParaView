## Improvements to autocomplete in _Python Shell_

ParaView's _Python Shell_ previously had basic autocomplete support that worked to complete the names of sources/filters/readers/writers, attributes of existing sources/filters/readers/writers, or names of free functions such as `SaveScreenshot` when calling them.

Now, autocomplete works in additional cases, including:

- Autocompleting property names inside a source/filter creation call - `s = Sphere(registra<TAB>` autocompletes to `s = Sphere(registrationName`

- Autocompleting keyword arguments in free function call - `Connect(ds_<TAB>` brings up a combobox with autocomplete choices `ds_host` and `ds_port`

- Multiple argument autocompletion - the first and subsequent arguments can be autocompleted, with two exceptions: 1). when a function call is not part of an assignment and 2). when a named argument is assigned a value with a decimal/period. These exceptions are limitations of the current implementation rather than fundamental limitations of autocomplete support in Python.
