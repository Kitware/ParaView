/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHAMRBlockIdScalars.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHAMRBlockIdScalars - Convert one cell array to a point array.
//
// .SECTION Description
// vtkCTHAMRBlockIdScalars converts one cell data array to point data.
// It can use ghost cells, but they do not do the best job at eliminating
// seams between different levels.  It also has an algorithm to look
// at neighboring blocks.  This is better but it is not perfect.
// The last remaining issue is the interpolation. Marching cubes boundary
// does not match trilinear of higher level. 

#ifndef __vtkCTHAMRBlockIdScalars_h
#define __vtkCTHAMRBlockIdScalars_h

#include "vtkCTHDataToCTHDataFilter.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHAMRBlockIdScalars : public vtkCTHDataToCTHDataFilter
{
public:
  static vtkCTHAMRBlockIdScalars *New();

  vtkTypeRevisionMacro(vtkCTHAMRBlockIdScalars,vtkCTHDataToCTHDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkCTHAMRBlockIdScalars();
  ~vtkCTHAMRBlockIdScalars();

  virtual void Execute();

private:
  void InternalImageDataCopy(vtkCTHAMRBlockIdScalars *src);

  vtkCTHAMRBlockIdScalars(const vtkCTHAMRBlockIdScalars&);  // Not implemented.
  void operator=(const vtkCTHAMRBlockIdScalars&);  // Not implemented.
};


#endif



