// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPythonCalculator
 * @brief   Evaluates a Python expression
 * vtkPythonCalculator uses Python to calculate an expression.
 * This filter depends heavily on the numpy and paraview.vtk modules.
 * To use the parallel functions, mpi4py is also necessary. The expression
 * is evaluated and the resulting scalar value or numpy array is added
 * to the output as an array. See numpy and paraview.vtk documentation
 * for the list of available functions.
 *
 * This filter tries to make it easy for the user to write expressions
 * by defining certain variables. The filter tries to assign each array
 * to a variable of the same name. If the name of the array is not a
 * valid Python variable, it has to be accessed through a dictionary called
 * arrays (i.e. arrays['array_name']). The points can be accessed using the
 * points variable.
 */

#ifndef vtkPythonCalculator_h
#define vtkPythonCalculator_h

#include "vtkDataObject.h"                         // for FIELD_ASSOCIATION_POINTS
#include "vtkPVVTKExtensionsFiltersPythonModule.h" //needed for exports
#include "vtkProgrammableFilter.h"

class VTKPVVTKEXTENSIONSFILTERSPYTHON_EXPORT vtkPythonCalculator : public vtkProgrammableFilter
{
public:
  vtkTypeMacro(vtkPythonCalculator, vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkPythonCalculator* New();

  ///@{
  /**
   * Which field data to get the arrays from. See
   * vtkDataObject::FieldAssociations for choices. The default
   * is FIELD_ASSOCIATION_POINTS.
   */
  vtkSetMacro(ArrayAssociation, int);
  vtkGetMacro(ArrayAssociation, int);
  ///@}

  ///@{
  /**
   * Set the text of the python expression to execute. This expression
   * must return a scalar value (which is converted to an array) or a
   * numpy array.
   */
  vtkSetMacro(Expression, std::string);
  vtkGetMacro(Expression, std::string);
  ///@}

  ///@{
  /**
   * Set the text of the python multiline expression. The expression must use an explicit return
   * statement for the result scalar value or numpy array.
   */
  vtkSetMacro(MultilineExpression, std::string);
  vtkGetMacro(MultilineExpression, std::string);
  ///@}

  ///@{
  /**
   * Set the name of the output array.
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  ///@}

  ///@{
  /**
   * Type of the result array.
   * Initial value is VTK_DOUBLE.
   */
  vtkGetMacro(ResultArrayType, int);
  vtkSetMacro(ResultArrayType, int);
  ///@}

  ///@{
  /**
   * If true, executes `MultilineExpression`, which is a multiline string representing a Python
   * script, ending by a return statement. Otherwise, evaluates `Expression`, a python expression.
   * Initial value is false.
   */
  vtkGetMacro(UseMultilineExpression, bool);
  vtkSetMacro(UseMultilineExpression, bool);
  ///@}

  /**
   * For internal use only.
   */
  static void ExecuteScript(void*);

protected:
  vtkPythonCalculator();
  ~vtkPythonCalculator() override;

  /**
   * For internal use only.
   */
  void Exec(const std::string&);

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  // overridden to allow multiple inputs to port 0
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // overridden to allow string substitutions for the Expression
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  // Creates whatever output data set type is selected.
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  std::string Expression;
  std::string MultilineExpression;
  bool UseMultilineExpression = false;

  char* ArrayName = nullptr;
  int ArrayAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
  int ResultArrayType = VTK_DOUBLE;

private:
  vtkPythonCalculator(const vtkPythonCalculator&) = delete;
  void operator=(const vtkPythonCalculator&) = delete;
};

#endif
