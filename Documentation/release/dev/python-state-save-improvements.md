## Python state save improvements

### Python state save animation

Python state now contains the animation state as well,
which includes the current animation time but also all the scene
properties and even the different animation tracks and keyframes.

### Python state save extract selection

Python state now contains the "Extract Selection" filter.

On a side note, you can also generate selections in Python scripts
that will not appear in the pipeline with the new function from
`selection.py` that is used internally when saving a python state.

```py
from paraview.simple import *

idSelection = CreateSelection(proxyname='IDSelectionSource', registrationName='selection_sources.1', IDs=[0, 7628])
```
