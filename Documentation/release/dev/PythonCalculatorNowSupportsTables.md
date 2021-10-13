# Add Table support for the Python Calculator

Accept *vtkTable* inputs for the *PythonCalculator* filter. Basic Python operations can be performed on the columns of one or multiple input table(s). If the *Copy Arrays* property is *ON* when the result of the Python operation happens to be a scalar value, the output will be a column whose values are all set to this scalar result. When using multiple input tables, the data may be accessed using the syntax : `inputs[0].RowData["column"] ` .
