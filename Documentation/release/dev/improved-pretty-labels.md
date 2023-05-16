## Fix and improve label creation for property and proxy name

When no label has been defined for a xml property or proxy,
`vtkSMObject::CreatePrettyLabel()` creates a label
from its Camel case name. It was previously done using
`vtkSMProperty::CreateNewPrettyLabel()` which is not deprecated.

* The method now splits better the name to also split words
following multiple uppercase letters.
* Multiple uppercase at the beginning of the name does not
create an incorrect space between first and second letter
anymore.
* Proxy names are now prettyfied, it was not the case before.
