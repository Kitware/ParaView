## Python state file fixes

Custom names for pipeline objects are now recorded in the python state file.

Image names and other filenames, used for background images, and during
tracing of the `SaveData` and `ExportView` commands, are now escaped properly
on Windows so backslashes in the path don't cause parse errors.
