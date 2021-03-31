# better-fortran-catalyst-bindings

Fortran bindings to Catalyst APIs has been improved. There is now a `catalyst`
and `catalyst_python` module available with namespaced APIs.

## `catalyst` module

  - `coprocessorinitialize()` is now `catalyst_initialize()`
  - `coprocessorfinalize()` is now `catalyst_finalize()`
  - `requestdatadescription(step, time, need)` is now `need = catalyst_request_data_description(step, time)`
  - `needtocreategrid(need)` is now `need = catalyst_need_to_create_grid()`
  - `coprocess()` is now `catalyst_process()`

The `need` return values are of type `logical` rather than an integer.

## `catalyst_python` module

  - `coprocessorinitializewithpython(filename, len)` is now `catalyst_initialize_with_python(filename)`
  - `coprocessoraddpythonscript(filename, len)` is now `catalyst_add_python_script(filename)`
