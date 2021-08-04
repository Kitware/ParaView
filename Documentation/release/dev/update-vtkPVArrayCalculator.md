#Update vtkPVArrayCalculator

The goal of this MR is to address the known issues https://gitlab.kitware.com/paraview/paraview/-/issues/20030 and https://gitlab.kitware.com/paraview/paraview/-/issues/20602.

Those fixes were possible thanks to the MR https://gitlab.kitware.com/vtk/vtk/-/merge_requests/8182/ which addressed what happens when a variable name is not sanitized from the side of the ``vtkArrayCalculator``.
``vtkPVArrayCalculator`` ensures now that the arrays that can be used must have a valid variable name.
``pqCalculatorWidget`` ensures now that the arrays that the user can select from the drop-down menus can actually be used.

A variable name is considered valid if it's sanitized or enclosed in quotes:
* If the array name is named `Array_B` (**therefore sanitized**) then the added variables that can be used in the expression are `Array_B` & `"Array_B"`
* If the array name is named `"Test_1"` (**therefore not sanitized but enclosed in quotes**) then the added variables that can be used in the expression are `"Test_1"`
* If the array name is named `Pressure (dyn/cm^2^)` or `1_test` (**therefore neither sanitized nor enclosed in quotes**) then the added variables that can be used in the expression are `"Pressure (dyn/cm^2^)"` or `"1_test"` respectively.
