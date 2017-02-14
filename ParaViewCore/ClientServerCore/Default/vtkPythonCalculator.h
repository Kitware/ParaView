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

#include "vtkPVClientServerCoreDefaultModule.h" //needed for exports
#include "vtkProgrammableFilter.h"

class VTKPVCLIENTSERVERCOREDEFAULT_EXPORT vtkPythonCalculator : public vtkProgrammableFilter
{
public:
  vtkTypeMacro(vtkPythonCalculator, vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkPythonCalculator* New();

  //@{
  /**
   * Which field data to get the arrays from. See
   * vtkDataObject::FieldAssociations for choices. The default
   * is FIELD_ASSOCIATION_POINTS.
   */
  vtkSetMacro(ArrayAssociation, int);
  vtkGetMacro(ArrayAssociation, int);
  //@}

  //@{
  /**
   * Set the text of the python expression to execute. This expression
   * must return a scalar value (which is converted to an array) or a
   * numpy array.
   */
  vtkSetStringMacro(Expression) vtkGetStringMacro(Expression)
    //@}

    //@{
    /**
     * Set the name of the output array.
     */
    vtkSetStringMacro(ArrayName) vtkGetStringMacro(ArrayName)
    //@}

    /**
     * For internal use only.
     */
    static void ExecuteScript(void*);

protected:
  vtkPythonCalculator();
  ~vtkPythonCalculator();

  /**
   * For internal use only.
   */
  void Exec(const char*);

  virtual int FillOutputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // overridden to allow multiple inputs to port 0
  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // DeExpressionion:
  // Creates whatever output data set type is selected.
  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  char* Expression;
  char* ArrayName;
  int ArrayAssociation;

private:
  vtkPythonCalculator(const vtkPythonCalculator&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPythonCalculator&) VTK_DELETE_FUNCTION;
};

#endif
