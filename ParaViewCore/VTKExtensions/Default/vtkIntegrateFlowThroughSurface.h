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
/**
 * @class   vtkIntegrateFlowThroughSurface
 * @brief   Integrates vector dot normal.
 *
 * First this filter finds point normals for a surface.  It
 * Takes a point vector field from the input and computes the
 * dot product with the normal.  It then integrates this dot value
 * to get net flow through the surface.
*/

#ifndef vtkIntegrateFlowThroughSurface_h
#define vtkIntegrateFlowThroughSurface_h

#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIdList;
class vtkDataSetAttributes;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkIntegrateFlowThroughSurface
  : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkIntegrateFlowThroughSurface, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkIntegrateFlowThroughSurface* New();

protected:
  vtkIntegrateFlowThroughSurface();
  ~vtkIntegrateFlowThroughSurface();

  // Usual data generation method
  // Usual data generation method
  virtual int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;
  virtual int RequestUpdateExtent(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) VTK_OVERRIDE;

  virtual int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive() VTK_OVERRIDE;

  vtkDataSet* GenerateSurfaceVectors(vtkDataSet* input);

private:
  vtkIntegrateFlowThroughSurface(const vtkIntegrateFlowThroughSurface&) VTK_DELETE_FUNCTION;
  void operator=(const vtkIntegrateFlowThroughSurface&) VTK_DELETE_FUNCTION;
};

#endif
