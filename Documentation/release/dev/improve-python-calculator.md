## Improved expression editing in **Python Calculator**

The **Python Calculator** panel now provides helpers to build expressions without memorizing the syntax.

- **Input**: selects which connected input to browse arrays from. Disabled when only one input is connected. When **Use
  named inputs** is checked, generated accessors use the dataset name (e.g. `inputs["Sphere"].PointData["Velocity"]`)
  instead of numeric indexing (e.g. `inputs[0].PointData["Velocity"]`).
- **Array**: lists arrays from the selected input, optionally filtered to the current **Array Association**. Clicking
  inserts the appropriate accessor at the cursor.
- **Function**: searchable list of Python functions available in the calculator's namespace, with a one-line summary
  per entry. Hovering shows the full documentation. Double-clicking inserts `function()` at the cursor with the cursor
  placed between the parentheses.

The default **Array Name** is now `Result` (capitalized), consistent with the regular **Calculator** filter.
Backwards compatibility has been handled.

`vtkPVArrayInformation` gains an `IsGlobal` flag to identify arrays living in root-level composite field data, used
by **Python Calculator** to generate correct accessors for global arrays.

![Python Calculator panel showing the Input, Array, and Function pickers](improved-python-calculator.png)
