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
/**
 * @class   vtkMergeCompositeDataSet
 * @brief   Make a vtkPolyData with a vertex on each
 * point.
 *
 *
 *
 * This filter throws away all of the cells in the input and replaces them with
 * a vertex on each point. This filter may take a graph, a point set or a
 * CompositeDataSet as input.
 *
*/

#ifndef vtkMergeCompositeDataSet_h
#define vtkMergeCompositeDataSet_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkVertexGlyphFilter.h"

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkMergeCompositeDataSet : public vtkVertexGlyphFilter
{
public:
  vtkTypeMacro(vtkMergeCompositeDataSet, vtkVertexGlyphFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkMergeCompositeDataSet* New();

protected:
  vtkMergeCompositeDataSet();
  ~vtkMergeCompositeDataSet() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  vtkMergeCompositeDataSet(const vtkMergeCompositeDataSet&) = delete;
  void operator=(const vtkMergeCompositeDataSet&) = delete;
};

#endif // vtkMergeCompositeDataSet_h
