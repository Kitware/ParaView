# Treeview header click to sort

A few tree view widgets, like `pqArrayStatusPropertyWidget` and
`pqDataAssemblyPropertyWidget` now allow sorting by clicking on the column
headers, as well as via the existing triangle menu. This follows standard QT
conventions for sorting tables. Previously clicking the header checked or
unchecked all items in the table - this behavior is still available by
clicking the checkbox in the header.
