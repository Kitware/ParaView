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
// .NAME vtkPythonAnnotationFilter
// .SECTION Description
// This filter allow the user to write an expression that can be use to
// extract any field information and represent the result as a text annotation
// in the 3D view

#ifndef __vtkPythonAnnotationFilter_h
#define __vtkPythonAnnotationFilter_h

#include "vtkPVClientServerCoreCoreModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class vtkStdString;

class VTKPVCLIENTSERVERCORECORE_EXPORT vtkPythonAnnotationFilter : public vtkTableAlgorithm
{
public:
  static vtkPythonAnnotationFilter* New();
  vtkTypeMacro(vtkPythonAnnotationFilter, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Allow the user to customize the output annotation based on some arbitrary
  // content processing
  //
  // Here is the set of preset variable available to you when you write your
  // expression.
  //   - input
  //   - input.PointData['myArray']
  //   - input.CellData['myArray']
  //   - input.FieldData['myArray']
  //   - t_value
  //   - t_steps
  //   - t_range
  //   - t_index
  //
  // Here is a set of common expressions:
  //  - "Momentum %s" % str(Momentum[available_timesteps.index(provided_time)])
  //
  vtkSetStringMacro(PythonExpression);
  vtkGetStringMacro(PythonExpression);

  // Description:
  // Set the value that is going to be printed to the output. This is an
  // internal method and should not be called directly. It is called by the
  // python script executed internally to pass the computed annotation value
  // back to the filter.
  void SetAnnotationValue(const char* value);
  vtkGetStringMacro(AnnotationValue);
//BTX
protected:
  vtkPythonAnnotationFilter();
  ~vtkPythonAnnotationFilter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(vtkInformation* request,
                          vtkInformationVector** inputVector,
                          vtkInformationVector* outputVector);

  // Description:
  // For internal use only.
  void EvaluateExpression();

  char* AnnotationValue;
  char* PythonExpression;

  // Used internally to store time informations for Python
  vtkSetStringMacro(TimeInformations);
  char* TimeInformations;

private:
  vtkPythonAnnotationFilter(const vtkPythonAnnotationFilter&); // Not implemented
  void operator=(const vtkPythonAnnotationFilter&); // Not implemented

  // Description: 
  // For internal use only.
  static void ExecuteScript(void *);
//ETX
};

#endif
