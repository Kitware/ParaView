# Command line arguments and ParaViewPython executables

A standard Python executable has several command line options arguments that often used
when running Python scripts. ParaView's python executables like `pvbatch` and
`pvpython` which are often used to run ParaView Python scripts. Traditionally,
however, these ParaView python executables have provided no means of passing
arguments to the Python interpreter itself. We address this issue with these
changes.

Now, both `pvpython` and `pvbatch` pass all unknown arguments to the Python
interpreter. Thus, to simply run the demo script from the `paraview.demos.demo1`
module, one can do the following:

```bash
> ./bin/pvpython -m paraview.demos.demo1

```

You can also explicit pass all subsequent arguments to the Python interpreter
using the `--` separator.

For example:

```bash
# here, --help is parsed and interpretted by ParaView
> ./bin/pvpython --help

  --connect-id=opt  Set the ID of the server and client to make sure they match.
                    0 is reserved to imply none specified.
  --cslog=opt  ClientServerStream log file.
  ...

# here, --help is instead processed by the Python interpreter used by ParaView
> ./bin/pvpython -- --help
usage: ./bin/pvpython [option] ... [-c cmd | -m mod | file | -] [arg] ...
Options and arguments (and corresponding environment variables):
-b     : issue warnings about str(bytes_instance), str(bytearray_instance)
         and comparing bytes/bytearray with str. (-bb: issue errors)
...

```
