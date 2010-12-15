/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPrismTableToPolyData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPrismTableToPolyData - filter used to convert a vtkTable to a vtkPolyData
// consisting of vertices
// .SECTION Description
// vtkPrismTableToPolyData is a filter used to convert a vtkTable  to a vtkPolyData
// consisting of vertices. Each vertice is in its own cell. In addition the remaining arrays are added as cell data.

#ifndef __vtkPrismTableToPolyData_h
#define __vtkPrismTableToPolyData_h

#include "vtkTableToPolyData.h"

class  vtkPrismTableToPolyData : public vtkTableToPolyData
{
public:
  static vtkPrismTableToPolyData* New();
  vtkTypeMacro(vtkPrismTableToPolyData, vtkTableToPolyData);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the column to use as the Y coordinate for the points.
  // Default is 0.
  vtkSetStringMacro(GobalElementIdColumn);
  vtkGetStringMacro(GobalElementIdColumn);
//BTX
protected:
  vtkPrismTableToPolyData();
  ~vtkPrismTableToPolyData();

  char* GobalElementIdColumn;
  // Description:
  // Convert input vtkTable to vtkPolyData.
  virtual int RequestData(vtkInformation* request,
    vtkInformationVector** inputVector, vtkInformationVector* outputVector);

private:
  vtkPrismTableToPolyData(const vtkPrismTableToPolyData&); // Not implemented.
  void operator=(const vtkPrismTableToPolyData&); // Not implemented.
//ETX
};

#endif
