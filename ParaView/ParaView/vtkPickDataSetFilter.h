/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPickDataSetFilter.h
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
// .NAME vtkPickDataSetFilter - pick the closest point or cell
// .SECTION Description
// vtkPickDataSetFilter is a filter that takes a dataset and a point
// in 3D space and return a dataset with the closest point to the
// sample point or the closets cell including all the points of the
// cell.
// 
// .SECTION Caveats

// .SECTION See Also
// vtkTensorGlyph

#ifndef __vtkPickDataSetFilter_h
#define __vtkPickDataSetFilter_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class VTK_GRAPHICS_EXPORT vtkPickDataSetFilter : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkPickDataSetFilter,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkPickDataSetFilter *New();

  // Description:
  // Set or get the sample point around which we will search for
  // closest point or cell.
  vtkSetVector3Macro(SamplePoint, float);
  vtkGetVector3Macro(SamplePoint, float);

  // Description:
  // Select wether to search cells or points.
  vtkSetClampMacro(SearchCells, int, 0, 1);
  vtkBooleanMacro(SearchCells, int);
  vtkGetMacro(SearchCells, int);

protected:
  vtkPickDataSetFilter();
  ~vtkPickDataSetFilter();

  void Execute();

  float SamplePoint[3];
  int SearchCells;

private:
  vtkPickDataSetFilter(const vtkPickDataSetFilter&);  // Not implemented.
  void operator=(const vtkPickDataSetFilter&);  // Not implemented.
};


#endif
