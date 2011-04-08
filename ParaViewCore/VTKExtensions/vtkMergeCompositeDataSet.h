/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeCompositeDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkMergeCompositeDataSet - Make a vtkPolyData with a vertex on each
// point.
//
// .SECTION Description
//
// This filter throws away all of the cells in the input and replaces them with
// a vertex on each point. This filter may take a graph, a point set or a 
// CompositeDataSet as input.
//

#ifndef __vtkMergeCompositeDataSet_h
#define __vtkMergeCompositeDataSet_h

#include "vtkVertexGlyphFilter.h"

class VTK_EXPORT vtkMergeCompositeDataSet : public vtkVertexGlyphFilter
{
public:
  vtkTypeMacro(vtkMergeCompositeDataSet, vtkVertexGlyphFilter);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkMergeCompositeDataSet *New();

protected:
  vtkMergeCompositeDataSet();
  virtual ~vtkMergeCompositeDataSet();

  virtual int RequestData(vtkInformation *,
                          vtkInformationVector **, vtkInformationVector *);
  virtual int FillInputPortInformation(int, vtkInformation *);

private:
  vtkMergeCompositeDataSet(const vtkMergeCompositeDataSet &); // Not implemented
  void operator=(const vtkMergeCompositeDataSet &);    // Not implemented
};

#endif //__vtkMergeCompositeDataSet_h
