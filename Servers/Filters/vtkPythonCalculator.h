/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPythonCalculator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPythonCalculator - Evaluates a Python expression
// vtkPythonCalculator uses Python to calculate an expression.
// This filter depends heavily on the numpy and paraview.vtk modules.
// To use the parallel functions, mpi4py is also necessary. The expression
// is evaluated and the resulting scalar value or numpy array is added
// to the output as an array. See numpy and paraview.vtk documentation
// for the list of available functions.
// 
// This filter tries to make it easy for the user to write expressions
// by defining certain variables. The filter tries to assign each array
// to a variable of the same name. If the name of the array is not a 
// valid Python variable, it has to be accessed through a dictionary called
// arrays (i.e. arrays['array_name']). The points can be accessed using the
// points variable.

#ifndef __vtkPythonCalculator_h
#define __vtkPythonCalculator_h

#include "vtkProgrammableFilter.h"

class VTK_EXPORT vtkPythonCalculator : public vtkProgrammableFilter
{
public:
  vtkTypeMacro(vtkPythonCalculator,vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkPythonCalculator *New();

  // Description:
  // Which field data to get the arrays from. See
  // vtkDataObject::FieldAssociations for choices. The default
  // is FIELD_ASSOCIATION_POINTS.
  vtkSetMacro(ArrayAssociation, int);
  vtkGetMacro(ArrayAssociation, int);

  // Description:
  // Controls whether the input arrays are copied to the output.
  // True by default.
  vtkSetMacro(CopyArrays, bool);
  vtkGetMacro(CopyArrays, bool);
  vtkBooleanMacro(CopyArrays, bool);

  // Description: 
  // Set the text of the python expression to execute. This expression
  // must return a scalar value (which is converted to an array) or a
  // numpy array.
  vtkSetStringMacro(Expression)
  vtkGetStringMacro(Expression)

  // Description: 
  // Set the name of the output array.
  vtkSetStringMacro(ArrayName)
  vtkGetStringMacro(ArrayName)

  // Description: 
  // For internal use only.
  static void ExecuteScript(void *);

protected:
  vtkPythonCalculator();
  ~vtkPythonCalculator();

  // Description:
  // For internal use only.
  void Exec(const char*, const char*);

  virtual int FillOutputPortInformation(int port, vtkInformation* info);

  //overridden to allow multiple inputs to port 0
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // DeExpressionion:
  // Creates whatever output data set type is selected.
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  char *Expression;
  char *ArrayName;
  int ArrayAssociation;
  bool CopyArrays;

private:
  vtkPythonCalculator(const vtkPythonCalculator&);  // Not implemented.
  void operator=(const vtkPythonCalculator&);  // Not implemented.
};

#endif
