/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxCellDataToPointData.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkHierarchicalBoxCellDataToPointData -
// .SECTION Description

#ifndef __vtkHierarchicalBoxCellDataToPointData_h
#define __vtkHierarchicalBoxCellDataToPointData_h

#include "vtkHierarchicalBoxToHierarchicalBoxFilter.h"

class vtkDataObject;

class VTK_EXPORT vtkHierarchicalBoxCellDataToPointData : 
  public vtkHierarchicalBoxToHierarchicalBoxFilter
{
public:
  static vtkHierarchicalBoxCellDataToPointData *New();

  vtkTypeRevisionMacro(vtkHierarchicalBoxCellDataToPointData,
                       vtkHierarchicalBoxToHierarchicalBoxFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Control whether the input cell data is to be passed to the output. If
  // on, then the input cell data is passed through to the output; otherwise,
  // only generated point data is placed into the output.
  vtkSetMacro(PassCellData,int);
  vtkGetMacro(PassCellData,int);
  vtkBooleanMacro(PassCellData,int);

protected:
  vtkHierarchicalBoxCellDataToPointData();
  ~vtkHierarchicalBoxCellDataToPointData();

  virtual void ExecuteData(vtkDataObject*);

  int PassCellData;
private:
  vtkHierarchicalBoxCellDataToPointData(
    const vtkHierarchicalBoxCellDataToPointData&);  // Not implemented.
  void operator=(
    const vtkHierarchicalBoxCellDataToPointData&);  // Not implemented.
};


#endif



