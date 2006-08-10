/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellSelect.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCellSelect - A DUMMY ALGORITHM FOR A GUI FILTER TO MAKE FOR DEBUG
// .SECTION Description

#ifndef __vtkCellSelect_h
#define __vtkCellSelect_h

#include "vtkPolyDataAlgorithm.h"

class VTK_EXPORT vtkCellSelect : public vtkPolyDataAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkCellSelect,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Constructor
  static vtkCellSelect *New();


protected:
  vtkCellSelect();
  ~vtkCellSelect();

  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:
  vtkCellSelect(const vtkCellSelect&);  // Not implemented.
  void operator=(const vtkCellSelect&);  // Not implemented.
};

#endif
