/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPythonExtractSelection
// .SECTION Description
//

#ifndef __vtkPythonExtractSelection_h
#define __vtkPythonExtractSelection_h

#include "vtkProgrammableFilter.h"

class vtkCharArray;
class vtkDataSet;
class vtkTable;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPythonExtractSelection : public vtkProgrammableFilter
{
public:
  static vtkPythonExtractSelection* New();
  vtkTypeMacro(vtkPythonExtractSelection, vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Which field data to get the arrays from. See
  // vtkDataObject::FieldAssociations for choices. The default
  // is FIELD_ASSOCIATION_POINTS.
  vtkSetMacro(ArrayAssociation, int);
  vtkGetMacro(ArrayAssociation, int);

  // Description:
  // Set the text of the python expression to execute. This expression
  // must return a scalar value (which is converted to an array) or a
  // numpy array.
  vtkSetStringMacro(Expression)
  vtkGetStringMacro(Expression)

  // Description:
  // Internal method.
  vtkDataObject* ExtractElements(vtkDataObject* data, vtkCharArray* mask);

//BTX
protected:
  vtkPythonExtractSelection();
  ~vtkPythonExtractSelection();

  // Description:
  // For internal use only.
  void Exec();
  
  vtkUnstructuredGrid* ExtractPoints(vtkDataSet* data, vtkCharArray* mask);
  vtkUnstructuredGrid* ExtractCells(vtkDataSet* data, vtkCharArray* mask);
  vtkTable* ExtractElements(vtkTable* data, vtkCharArray* mask);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  // Description:
  // Creates whatever output data set type is selected.
  virtual int RequestDataObject(vtkInformation* request, 
                                vtkInformationVector** inputVector, 
                                vtkInformationVector* outputVector);

  int ArrayAssociation;
  char* Expression;

private:
  vtkPythonExtractSelection(const vtkPythonExtractSelection&); // Not implemented
  void operator=(const vtkPythonExtractSelection&); // Not implemented

  // Description: 
  // For internal use only.
  static void ExecuteScript(void *);
//ETX
};

#endif
