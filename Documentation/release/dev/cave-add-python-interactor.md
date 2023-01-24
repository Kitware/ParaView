## CAVEInteraction: Add customizable python interactor styles

The CAVEInteraction plugin has been enhanced with a customizable Python
interactor style.  If the user chooses the new interaction type "Python"
from the interaction types drop-down list, they are then presented with
a file selection widget in the proxy properties, when that proxy is
selected.

The user must provide, within the selected python file, a method named
`create_interactor_style()` that takes no arguments and contructs/returns a
python object implementing the handler methods normally implemented by
subclasses of `vtkSMVRInteractorStyleProxy`.

There is an example python file located in:

```
Examples/VR/CustomizablePythonInteractorStyle.py
```
