/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkM2NCollect.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkM2NCollect - For distributed tiled displays.
// .DESCRIPTION
// 


#ifndef __vtkM2NCollect_h
#define __vtkM2NCollect_h

#include "vtkPolyDataToPolyDataFilter.h"


class VTK_EXPORT vtkM2NCollect : public vtkPolyDataToPolyDataFilter
{
public:
  static vtkM2NCollect *New();
  vtkTypeRevisionMacro(vtkM2NCollect, vtkPolyDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  
protected:
  vtkM2NCollect();
  ~vtkM2NCollect();

  // Data generation method
  void ComputeInputUpdateExtents(vtkDataObject *output);
  void Execute();
  void ExecuteInformation();

  int ShuffleSizes(int size);
  void Shuffle(int inSize, char* inBuf, int outSize, char* outBuf);

private:
  vtkM2NCollect(const vtkM2NCollect&); // Not implemented
  void operator=(const vtkM2NCollect&); // Not implemented
};

#endif

