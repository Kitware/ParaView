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
class vtkSelection;
class vtkUnstructuredGrid;

class VTK_EXPORT vtkPythonExtractSelection : public vtkProgrammableFilter
{
public:
  static vtkPythonExtractSelection* New();
  vtkTypeMacro(vtkPythonExtractSelection, vtkProgrammableFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Convenience method to specify the selection connection (2nd input
  // port)
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
    {
    this->SetInputConnection(1, algOutput);
    }

  // Description:
  // Internal method.
  vtkDataObject* ExtractElements(vtkDataObject* data, vtkSelection* selection, vtkCharArray* mask);

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

private:
  vtkPythonExtractSelection(const vtkPythonExtractSelection&); // Not implemented
  void operator=(const vtkPythonExtractSelection&); // Not implemented

  // Description: 
  // For internal use only.
  static void ExecuteScript(void *);
//ETX
};

#endif
