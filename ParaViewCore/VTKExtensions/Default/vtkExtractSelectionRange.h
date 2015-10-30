/*=========================================================================

  Program:   ParaView
  Module:    vtkExtractSelectionRange.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectionRange - compute the range of the input selection.
// .SECTION Description
// vtkExtractSelectionRange is used to compute the range of an input selection.
// This is an internal filter designed to be used to "compute visible range" for
// a dataset.

#ifndef vtkExtractSelectionRange_h
#define vtkExtractSelectionRange_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkTableAlgorithm.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkExtractSelectionRange : public vtkTableAlgorithm
{
public:
  static vtkExtractSelectionRange* New();
  vtkTypeMacro(vtkExtractSelectionRange, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the array used to compute the range.
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);

  // Description:
  // Set/Get the field type of the selection.
  vtkSetMacro(FieldType, int);
  vtkGetMacro(FieldType, int);

  // Description:
  // Set/Get the component used to compute the range.
  vtkSetMacro(Component, int);
  vtkGetMacro(Component, int);

  // Description:
  // Get the output range
  vtkGetVector2Macro(Range, double);

//BTX
protected:
  vtkExtractSelectionRange();
  ~vtkExtractSelectionRange();

  int FillInputPortInformation(int port, vtkInformation* info);
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

  int FieldType;
  int Component;
  char *ArrayName;
  double Range[2];

private:
  vtkExtractSelectionRange(const vtkExtractSelectionRange&); // Not implemented
  void operator=(const vtkExtractSelectionRange&); // Not implemented

//ETX
};

#endif
