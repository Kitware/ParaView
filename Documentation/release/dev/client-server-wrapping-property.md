# client-server-wrapping-property


* The `ClientServer` wrapping tool now uses the `WRAP_EXCLUDE_PYTHON` property
  rather than `WRAP_EXCLUDE`. It also now properly skips classes which cannot
  be wrapped with no-op wrappings (rather than failing as before). One
  limitation currently is that files with no classes still need excluded since
  the wrapper does not know what name to use when creating the no-op function.
