/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIntegrateFlowThroughSurface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIntegrateFlowThroughSurface - Integrates vector dot normal.
// .SECTION Description
// First this filter finds point normals for a surface.  It
// Takes a point vector field from the input and computes the
// dot product with the normal.  It then integrates this dot value
// to get net flow through the surface.

#ifndef __vtkIntegrateFlowThroughSurface_h
#define __vtkIntegrateFlowThroughSurface_h

#include "vtkDataSetToUnstructuredGridFilter.h"

class vtkDataSet;
class vtkIdList;
class vtkDataSetAttributes;

class VTK_EXPORT vtkIntegrateFlowThroughSurface : public vtkDataSetToUnstructuredGridFilter
{
public:
  vtkTypeRevisionMacro(vtkIntegrateFlowThroughSurface,vtkDataSetToUnstructuredGridFilter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkIntegrateFlowThroughSurface *New();
  
  // Description:
  // If you want to warp by an arbitrary vector array, then set its name here.
  // By default this is NULL and the filter will use the active vector array.
  vtkGetStringMacro(InputVectorsSelection);
  void SelectInputVectors(const char *fieldName) 
    {this->SetInputVectorsSelection(fieldName);}

protected:
  vtkIntegrateFlowThroughSurface();
  ~vtkIntegrateFlowThroughSurface();

  vtkSetStringMacro(InputVectorsSelection);
  char* InputVectorsSelection;

  // Usual data generation method
  void Execute();
  void ComputeInputUpdateExtent();

private:
  vtkIntegrateFlowThroughSurface(const vtkIntegrateFlowThroughSurface&); // Not implemented.
  void operator=(const vtkIntegrateFlowThroughSurface&);  // Not implemented.
};

#endif
