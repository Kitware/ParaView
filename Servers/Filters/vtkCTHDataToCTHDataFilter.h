/*=========================================================================

  Program:   ParaView
  Module:    vtkCTHDataToCTHDataFilter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCTHDataToCTHDataFilter - abstract filter class
// .SECTION Description
// vtkCTHDataToCTHDataFilter is an abstract filter class whose
// subclasses take as input datasets of type vtkCTHData and 
// generate polygonal data on output.

// .SECTION See Also
// vtkContourGrid

#ifndef __vtkCTHDataToCTHDataFilter_h
#define __vtkCTHDataToCTHDataFilter_h

#include "vtkCTHSource.h"

class vtkCTHData;


class VTK_EXPORT vtkCTHDataToCTHDataFilter : public vtkCTHSource
{
public:
  vtkTypeRevisionMacro(vtkCTHDataToCTHDataFilter,vtkCTHSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  virtual void SetInput(vtkCTHData *input);
  vtkCTHData *GetInput();
  
  // Description:
  // Do not let datasets return more than requested.
  virtual void ComputeInputUpdateExtents( vtkDataObject *output );

protected:
  vtkCTHDataToCTHDataFilter();
  ~vtkCTHDataToCTHDataFilter() {}
  
  virtual int FillInputPortInformation(int, vtkInformation*);
  
private:
  vtkCTHDataToCTHDataFilter(const vtkCTHDataToCTHDataFilter&);  // Not implemented.
  void operator=(const vtkCTHDataToCTHDataFilter&);  // Not implemented.
};

#endif


