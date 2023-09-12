# Improve support for FieldData in PythonCalculator

In the python calculator, FieldData are available with the full expression, like:
`inputs[0].FieldData['arrayName']`.
Now, the `ArrayAssociation` can be set to `Field Data`. This has two effects:
* `arrayName` can be used directly as a variable
* output array is added to the `FieldData` of the output.
