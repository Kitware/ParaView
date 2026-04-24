## Supports `.` in assembly node name

vtkDataAssembly now supports `.` in assembly node name.

This change slightly the behavior of GroupDataSet filter which
used to generate node names without a `.`, now it contains a `.`.

This can break existing state files or python script that would then
uses a selector property such as ExtractBlock filter.
