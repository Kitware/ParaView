/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHDataToPolyDataFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHDataToPolyDataFilter - abstract filter class
// .SECTION Description
// vtkCTHDataToPolyDataFilter is an abstract filter class whose
// subclasses take as input datasets of type vtkCTHData and 
// generate polygonal data on output.

// .SECTION See Also
// vtkContourGrid

#ifndef __vtkCTHDataToPolyDataFilter_h
#define __vtkCTHDataToPolyDataFilter_h

#include "vtkPolyDataSource.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHDataToPolyDataFilter : public vtkPolyDataSource
{
public:
  vtkTypeRevisionMacro(vtkCTHDataToPolyDataFilter,vtkPolyDataSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkCTHData *input);
  vtkCTHData *GetInput();
  
  // Description:
  // Do not let datasets return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkCTHDataToPolyDataFilter();
  ~vtkCTHDataToPolyDataFilter() {}
  
  virtual int FillInputPortInformation(int, vtkInformation*);
  
private:
  vtkCTHDataToPolyDataFilter(const vtkCTHDataToPolyDataFilter&);  // Not implemented.
  void operator=(const vtkCTHDataToPolyDataFilter&);  // Not implemented.
};

#endif


