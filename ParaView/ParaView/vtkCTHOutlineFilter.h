/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHOutlineFilter.h
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
// .NAME vtkCTHOutlineFilter - A source to test my new CTH AMR data object.
// .SECTION Description
// vtkCTHOutlineFilter is a collection of image datas.  All have the same dimensions.
// Each block has a different origin and spacing.  It uses mandelbrot
// to create cell data.  I scale the fractal array to look like a volme fraction.
// I may also add block id and level as extra cell arrays.

#ifndef __vtkCTHOutlineFilter_h
#define __vtkCTHOutlineFilter_h

#include "vtkCTHDataToPolyDataFilter.h"

class vtkCTHData;

class VTK_EXPORT vtkCTHOutlineFilter : public vtkCTHDataToPolyDataFilter
{
public:
  static vtkCTHOutlineFilter *New();

  vtkTypeRevisionMacro(vtkCTHOutlineFilter,vtkCTHDataToPolyDataFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkCTHOutlineFilter();
  ~vtkCTHOutlineFilter();

  virtual void Execute();

private:
  void InternalImageDataCopy(vtkCTHOutlineFilter *src);

private:
  vtkCTHOutlineFilter(const vtkCTHOutlineFilter&);  // Not implemented.
  void operator=(const vtkCTHOutlineFilter&);  // Not implemented.
};


#endif



