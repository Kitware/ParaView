/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkThickenLayeredCells.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThickenLayeredCells
// .SECTION Description
// vtkThickenLayeredCells is a filter that thickens cells processing them in
// layers (highest to lowest) using average thickeness. Both thickness and
// layers information is expected as cell-data in the input dataset.
// Currently this filter only supports wedges.

#ifndef vtkThickenLayeredCells_h
#define vtkThickenLayeredCells_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkThickenLayeredCells : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkThickenLayeredCells* New();
  vtkTypeMacro(vtkThickenLayeredCells, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // Description:
  // Enable/disable thickening.
  vtkSetMacro(EnableThickening, bool);
  vtkGetMacro(EnableThickening, bool);

protected:
  vtkThickenLayeredCells();
  ~vtkThickenLayeredCells();

  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  bool EnableThickening;

private:
  vtkThickenLayeredCells(const vtkThickenLayeredCells&) VTK_DELETE_FUNCTION;
  void operator=(const vtkThickenLayeredCells&) VTK_DELETE_FUNCTION;
};

#endif
