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

#include "vtkUnstructuredGridAlgorithm.h"

class vtkIdList;
class vtkDataSetAttributes;

class VTK_EXPORT vtkIntegrateFlowThroughSurface : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkIntegrateFlowThroughSurface,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkIntegrateFlowThroughSurface *New();
  
protected:
  vtkIntegrateFlowThroughSurface();
  ~vtkIntegrateFlowThroughSurface();

  // Usual data generation method
  // Usual data generation method
  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);
  virtual int RequestUpdateExtent(vtkInformation*,
                                  vtkInformationVector**,
                                  vtkInformationVector*);

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  vtkDataSet* GenerateSurfaceVectors(vtkDataSet* input);

private:
  vtkIntegrateFlowThroughSurface(const vtkIntegrateFlowThroughSurface&); // Not implemented.
  void operator=(const vtkIntegrateFlowThroughSurface&);  // Not implemented.
};

#endif
