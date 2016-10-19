/*=========================================================================

  Program:   ParaView
  Module:    vtkPistonInformationHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPistonInformationHelper - allows PVDataInformation to
// refer to vtkPistonDataObjects
//
// .SECTION Description
// This is a customization of vtkPistonInformationHelper that populates
// the corresponding vtkPVDataInformation with information from a
// PistonDataObject.

#ifndef vtkPistonInformationHelper_h
#define vtkPistonInformationHelper_h

#include "vtkPVDataInformationHelper.h"

class VTK_EXPORT vtkPistonInformationHelper : public vtkPVDataInformationHelper
{
public:
  static vtkPistonInformationHelper* New();
  vtkTypeMacro(vtkPistonInformationHelper, vtkPVDataInformationHelper);
  void PrintSelf(ostream& os, vtkIndent indent);

  const char* GetPrettyDataTypeString();

protected:
  vtkPistonInformationHelper();
  ~vtkPistonInformationHelper();

  bool ValidateType(vtkDataObject* data);

  // API to access information I fill the PVDataInformation I am friend of with
  double* GetBounds();
  int GetNumberOfDataSets();
  vtkTypeInt64 GetNumberOfCells();
  vtkTypeInt64 GetNumberOfPoints();
  vtkTypeInt64 GetNumberOfRows();

private:
  vtkPistonInformationHelper(const vtkPistonInformationHelper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPistonInformationHelper&) VTK_DELETE_FUNCTION;
};

#endif
