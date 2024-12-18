## Refactoring of simple.py

Over time this module grew to contain many functions. It was time to rework its structure to promote clarity and enable a deprecation path while empowering developers to create new and better helpers.

The new structure breaks the single simple.py file into a package split into a set of submodules with common responsibilities.
Deprecated methods are centralized within their own file for easy eventual removal.

This change also exposes a new set of methods (Set, Rename) on proxy directly to streamline their usage.

The code block below capture the usage of those new methods:

```python
from paraview.simple import *

cone_proxy = Cone()

# Update several properties at once
cone_proxy.Set(
    Radius=2,
    Center=(0,1,2),
    Height=5,
    # Below kwarg will print a warning
    # as the `Hello` property does not exist
    # for the `Cone` proxy
    Hello="World",
)

# Update source name in the GUI pipeline
cone_proxy.Rename("My Super Cone")
```
