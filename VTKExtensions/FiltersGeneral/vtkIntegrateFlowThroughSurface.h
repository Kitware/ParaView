// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkPVVTKExtensionsFiltersGeneralModule.h" //needed for exports
#include "vtkUnstructuredGridAlgorithm.h"

class vtkIdList;
class vtkDataSetAttributes;

class VTKPVVTKEXTENSIONSFILTERSGENERAL_EXPORT vtkIntegrateFlowThroughSurface
  : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkIntegrateFlowThroughSurface, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkIntegrateFlowThroughSurface* New();

protected:
  vtkIntegrateFlowThroughSurface();
  ~vtkIntegrateFlowThroughSurface() override;

  // Usual data generation method
  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Create a default executive.
  vtkExecutive* CreateDefaultExecutive() override;

  vtkDataSet* GenerateSurfaceVectors(vtkDataSet* input);

private:
  vtkIntegrateFlowThroughSurface(const vtkIntegrateFlowThroughSurface&) = delete;
  void operator=(const vtkIntegrateFlowThroughSurface&) = delete;
};

#endif
