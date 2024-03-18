## Proxy deprecation

### Mechanism
Full proxy deprecation is now possible, thanks to improvement in
the python backward compatibility module.
(see `_backwardscompatibilityhelper.get_deprecated_proxies()`).


### Renamed proxies
It allows to rename proxies, trying to use meaningful, shorter names.
Here is the list of renamed proxies:
 - `GhostCellsGenerator` becomes `GhostCells`
 - `AddFieldArrays` -> `FieldArraysFromFile`
