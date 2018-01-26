# Json based new meta file format for series added

vtkFileSeriesReader now supports a new meta file format based
on Json. This format has support for specifying time values.
The format (currently version 1.0) looks like the following

```json
{
"file-series-version" : "1.0",
"files" : [
{ "name" : "foo1.vtk", "time" : 0 },
{ "name" : "foo2.vtk", "time" : 5.5 },
{ "name" : "foo3.vtk", "time" : 11.2 }]
}
```

The reader will interpret any files that end with the .series
extension as a meta file. To add support for the meta file for
a reader in the readers description, add the extension.series
extension to the extension list. For example, for legacy VTK
files, the reader now supports .vtk and .vtk.series extensions.
