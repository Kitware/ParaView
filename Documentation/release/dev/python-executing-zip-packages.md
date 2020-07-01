## Executing Python packages in a zip archive

Python supports the ability to execute a Python package similar to standard
`.py` script. A `__main__.py` file inside the package is treated as the entry
point for such an invocation.

It's not uncommon to bundle a Python package (or module) in a zip archive. To import a package
inside a zip archive, the standard approach is to add the zip archive to the
Python module search path (i.e. `sys.path`) and then import the package using
its name.

To make this process a little easier, ParaView's Python executables i.e. pvbatch
and pvpython now support passing a zip archive on the command line instead of a
`.py` script. In that case, these executables will internally add the zip
archive to the Python module search path `sys.path` and attempt to load the
package with same name as the zip archive (without the extension).

For example:

```
> pvpython /tmp/sample.zip
```

This is same as the following:

```
> export PYTHONPATH=/tmp/sample.zip:$PYTHONPATH
> pvpython -m sample
```
