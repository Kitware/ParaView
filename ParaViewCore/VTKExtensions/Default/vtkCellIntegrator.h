/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellIntegrator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellIntegrator
 * @brief   Calculates length/area/volume of a cell
 *
 * vtkCellIntegrator is a helper class that calculates the
 * length/area/volume of a 1D/2D/3D cell. The calculation is exact for
 * lines, polylines, triangles, triangle strips, pixels, voxels, convex
 * polygons, quads and tetrahedra. All other 3D cells are triangulated
 * during volume calculation. In such cases, the result may not be exact.
*/

#ifndef vtkCellIntegrator_h
#define vtkCellIntegrator_h

#include "vtkObject.h"
#include "vtkPVVTKExtensionsDefaultModule.h" //needed for exports

class vtkDataSet;
class vtkIdList;

class VTKPVVTKEXTENSIONSDEFAULT_EXPORT vtkCellIntegrator : public vtkObject
{
public:
  vtkTypeMacro(vtkCellIntegrator, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Returns length/area/volume of a 1D/2D/3D cell given by cell id. If the
   * length/area/volume cannot be calculated (because of unsupposed cell
   * type), 0 is returned
   */
  static double Integrate(vtkDataSet* input, vtkIdType cellId);

protected:
  vtkCellIntegrator(){};
  ~vtkCellIntegrator() override{};

private:
  static double IntegratePolyLine(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds);
  static double IntegrateTriangleStrip(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds);
  static double IntegratePolygon(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds);
  static double IntegratePixel(vtkDataSet* input, vtkIdType cellId, vtkIdList* cellPtIds);
  static double IntegrateTriangle(
    vtkDataSet* input, vtkIdType cellId, vtkIdType pt1Id, vtkIdType pt2Id, vtkIdType pt3Id);
  static double IntegrateTetrahedron(vtkDataSet* input, vtkIdType cellId, vtkIdType pt1Id,
    vtkIdType pt2Id, vtkIdType pt3Id, vtkIdType pt4Id);
  static double IntegrateVoxel(vtkDataSet* input, vtkIdType cellId, vtkIdList* cellPtIds);
  static double IntegrateGeneral1DCell(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds);
  static double IntegrateGeneral2DCell(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds);
  static double IntegrateGeneral3DCell(vtkDataSet* input, vtkIdType cellId, vtkIdList* ptIds);

  vtkCellIntegrator(const vtkCellIntegrator&) = delete;
  void operator=(const vtkCellIntegrator&) = delete;
};

#endif
