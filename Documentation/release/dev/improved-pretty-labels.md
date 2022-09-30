## Fix and improve label creation from property name

When a label is missing in an xml proxy description,
`vtkSMProperty::CreateNewPrettyLabel()` creates a label
from its Camel case name.

* The method now splits better the name to also split words
following multiple uppercase letters.
* Multiple uppercase at the beginning of the name does not
create an incorrect space between first and second letter
anymore.
