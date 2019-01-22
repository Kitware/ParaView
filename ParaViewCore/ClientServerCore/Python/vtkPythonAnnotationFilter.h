/*=========================================================================

  Program:   ParaView
  Module:    vtkPythonAnnotationFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPythonAnnotationFilter
 * @brief   filter used to generate text annotation
 * from Python expressions.
 *
 * vtkPythonAnnotationFilter is designed to generate vtkTableAlgorithm with a
 * single string in it. The goal is that user will write a Python expression,
 * similar to an expression in Python Calculator (vtkPythonCalculator). The
 * generated result is converted to string and placed in the output.
 *
 * The variables available in the expression evaluation scope are as follows:
 * \li sanitized array names for all arrays in the chosen ArrayAssociation.
 * \li input: refers to the input dataset (wrapped as
 * vtk.numpy_interface.dataset_adapter.DataObject or subclass).
 * \li time_value: vtkDataObject::DATA_TIME_STEP() from input.
 * \li time_steps: vtkDataObject::TIME_STEPS() from the input, if any
 * \li time_range: vtkDataObject::TIME_RANGE() from the input, if any
 *
 * Examples of valid expressions are:
 * \li "Max temp is %s" % max(Temp)
*/

#ifndef vtkPythonAnnotationFilter_h
#define vtkPythonAnnotationFilter_h

#include "vtkPVClientServerCorePythonModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVCLIENTSERVERCOREPYTHON_EXPORT vtkPythonAnnotationFilter : public vtkTableAlgorithm
{
public:
  static vtkPythonAnnotationFilter* New();
  vtkTypeMacro(vtkPythonAnnotationFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the expression to evaluate.
   * Here is a set of common expressions:
   * - "Momentum %s" % str(Momentum[available_timesteps.index(provided_time)])
   */
  vtkSetStringMacro(Expression);
  vtkGetStringMacro(Expression);
  //@}

  //@{
  /**
   * Set the input array association. This dictates which array names are made
   * available in the namespace by default. You can still use
   * input.PointData['foo'] or input.CellData['bar'] explicitly to pick a
   * specific array in your expression.
   */
  vtkSetMacro(ArrayAssociation, int);
  vtkGetMacro(ArrayAssociation, int);
  //@}

  //@{
  /**
   * Get the value that is going to be printed to the output.
   */
  vtkGetStringMacro(ComputedAnnotationValue);
  //@}

  //------------------------------------------------------------------------------
  //@{
  /**
   * Get methods for use in annotation.py.
   * The values are only valid during RequestData().
   */
  vtkGetMacro(DataTimeValid, bool);
  vtkGetMacro(DataTime, double);
  //@}

  vtkGetMacro(NumberOfTimeSteps, int);
  double GetTimeStep(int index)
  {
    return (index < this->NumberOfTimeSteps ? this->TimeSteps[index] : 0.0);
  }

  vtkGetMacro(TimeRangeValid, bool);
  vtkGetVector2Macro(TimeRange, double);
  vtkGetObjectMacro(CurrentInputDataObject, vtkDataObject);
  void SetComputedAnnotationValue(const char* value);

protected:
  vtkPythonAnnotationFilter();
  ~vtkPythonAnnotationFilter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  virtual void EvaluateExpression();

  char* Expression;
  char* ComputedAnnotationValue;
  int ArrayAssociation;

private:
  vtkPythonAnnotationFilter(const vtkPythonAnnotationFilter&) = delete;
  void operator=(const vtkPythonAnnotationFilter&) = delete;

  bool DataTimeValid;
  double DataTime;
  int NumberOfTimeSteps;
  double* TimeSteps;
  bool TimeRangeValid;
  double TimeRange[2];
  vtkDataObject* CurrentInputDataObject;
};

#endif
