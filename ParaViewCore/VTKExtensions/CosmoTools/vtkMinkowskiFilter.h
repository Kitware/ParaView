/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkMinkowskiFilter.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   vtkMinkowskiFilter
 *
 *
 * Given as input a voronoi tessellation, stored in a vtkUnstructuredGrid, this
 * filter computes the Minkowski functionals on each cell.
*/

#ifndef vtkMinkowskiFilter_h
#define vtkMinkowskiFilter_h

#include "vtkPVVTKExtensionsCosmoToolsModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class vtkUnstructuredGrid;
class vtkCell;
class vtkDoubleArray;
class vtkPolyhedron;

class VTKPVVTKEXTENSIONSCOSMOTOOLS_EXPORT vtkMinkowskiFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkMinkowskiFilter* New();
  vtkTypeMacro(vtkMinkowskiFilter, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkMinkowskiFilter();
  ~vtkMinkowskiFilter();

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int FillOutputPortInformation(int, vtkInformation*);

private:
  vtkMinkowskiFilter(const vtkMinkowskiFilter&) = delete;
  void operator=(const vtkMinkowskiFilter&) = delete;

  void compute_mf(vtkUnstructuredGrid* ugrid, vtkDoubleArray* S, vtkDoubleArray* V,
    vtkDoubleArray* C, vtkDoubleArray* X, vtkDoubleArray* G, vtkDoubleArray* T, vtkDoubleArray* B,
    vtkDoubleArray* L, vtkDoubleArray* P, vtkDoubleArray* F);
  double compute_S(vtkPolyhedron* cell);                 // surface area
  double compute_V(vtkPolyhedron* cell);                 // volume1
  double compute_V(vtkUnstructuredGrid* ugrid, int cid); // volume2
  double compute_C(vtkPolyhedron* cell);                 // integrated mean curvature
  double compute_X(vtkPolyhedron* cell);                 // euler characteristic
  double compute_G(double X);                            // genus
  double compute_T(double V, double S);                  // thickness
  double compute_B(double S, double C);                  // breadth
  double compute_L(double C, double G);                  // length
  double compute_P(double B, double L);                  // planarity
  double compute_F(double B, double T);                  // filamenarity

  void compute_normal(vtkCell* face, double normal[3]);
  int compute_epsilon(vtkCell* f1, vtkCell* f2, vtkCell* e);
  double compute_face_area(vtkCell* face);
  double compute_edge_length(vtkCell* edge);
  double compute_face_angle(vtkCell* f1, vtkCell* f2);
};

#endif //  vtkMinkowskiFilter_h
