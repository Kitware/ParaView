## Add pqAdjustToCurrentComboBox

`pqAdjustToCurrentComboBox` is a new `QComboBox` specialization class whose size hint is computed
from the currently selected item rather than the widest item in the model (which is what
`QComboBox::AdjustToContents` bases its size hint on). This lets a combo box grow and shrink as the selection changes
instead of always reserving space for the longest possible entry.

`pqSelectionQueryPropertyWidget`'s **Term** and **Operator** dropdowns (used in the _Find Data_ selection query
editor) have been updated to use it. These dropdowns can list entries built from array names, which are often long
and vary a lot in length from one array to the next. Previously, adding even one long array name would permanently
widen the dropdown for the whole session, wasting panel space whenever a short array name was selected afterward.
With this change the dropdown only takes up as much room as the current selection needs.
