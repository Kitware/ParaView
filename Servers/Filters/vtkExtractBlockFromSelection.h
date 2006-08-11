/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractBlockFromSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractBlockFromSelection -
// .SECTION Description

#ifndef __vtkExtractBlockFromSelection_h
#define __vtkExtractBlockFromSelection_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkExecutive;

class VTK_EXPORT vtkExtractBlockFromSelection : public vtkUnstructuredGridAlgorithm 
{
public:
  static vtkExtractBlockFromSelection *New();
  vtkTypeRevisionMacro(vtkExtractBlockFromSelection,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkExtractBlockFromSelection();
  ~vtkExtractBlockFromSelection();

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();
  virtual int FillInputPortInformation(int, vtkInformation *);
  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
private:
  vtkExtractBlockFromSelection(const vtkExtractBlockFromSelection&);  // Not implemented.
  void operator=(const vtkExtractBlockFromSelection&);  // Not implemented.
};

#endif


