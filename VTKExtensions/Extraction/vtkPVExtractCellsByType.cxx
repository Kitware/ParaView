// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPVExtractCellsByType.h"

#include <vtkCellType.h>
#include <vtkCommand.h>
#include <vtkDataArraySelection.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <map>
#include <string>

vtkStandardNewMacro(vtkPVExtractCellsByType);

namespace
{
static const std::map<std::string, unsigned int> SupportedCellTypes = { { "Vertex", VTK_VERTEX },
  { "Polyvertex", VTK_POLY_VERTEX }, { "Line", VTK_LINE }, { "Polyline", VTK_POLY_LINE },
  { "Triangle", VTK_TRIANGLE }, { "Triangle Strip", VTK_TRIANGLE_STRIP },
  { "Polygon", VTK_POLYGON }, { "Pixel", VTK_PIXEL }, { "Quadrilateral", VTK_QUAD },
  { "Tetrahedron", VTK_TETRA }, { "Voxel", VTK_VOXEL }, { "Hexahedron", VTK_HEXAHEDRON },
  { "Wedge", VTK_WEDGE }, { "Pyramid", VTK_PYRAMID }, { "Pentagonal Prism", VTK_PENTAGONAL_PRISM },
  { "Hexagonal Prism", VTK_HEXAGONAL_PRISM }, { "Polyhedron", VTK_POLYHEDRON },
  { "Quadratic Edge", VTK_QUADRATIC_EDGE }, { "Quadratic Triangle", VTK_QUADRATIC_TRIANGLE },
  { "Quadratic Quadrilateral", VTK_QUADRATIC_QUAD }, { "Quadratic Polygon", VTK_QUADRATIC_POLYGON },
  { "Quadratic Tetrahedron", VTK_QUADRATIC_TETRA },
  { "Quadratic Hexahedron", VTK_QUADRATIC_HEXAHEDRON }, { "Quadratic Wedge", VTK_QUADRATIC_WEDGE },
  { "Quadratic Pyramid", VTK_QUADRATIC_PYRAMID },
  { "Bi-Quadratic Quadrilateral", VTK_BIQUADRATIC_QUAD },
  { "Tri-Quadratic Hexahedron", VTK_TRIQUADRATIC_HEXAHEDRON },
  { "Tri-Quadratic Pyramid", VTK_TRIQUADRATIC_PYRAMID },
  { "Quadratic Linear Quadrilateral", VTK_QUADRATIC_LINEAR_QUAD },
  { "Quadratic Linear Wedge", VTK_QUADRATIC_LINEAR_WEDGE },
  { "Bi-Quadratic Quadratic Wedge", VTK_BIQUADRATIC_QUADRATIC_WEDGE },
  { "Bi-Quadratic Quadratic Hexahedron", VTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON },
  { "Bi-Quadratic Triangle", VTK_BIQUADRATIC_TRIANGLE }, { "Cubic Line", VTK_CUBIC_LINE },
  { "Lagrange Curve", VTK_LAGRANGE_CURVE }, { "Lagrange Triangle", VTK_LAGRANGE_TRIANGLE },
  { "Lagrange Quadrilateral", VTK_LAGRANGE_QUADRILATERAL },
  { "Lagrange Tetrahedron", VTK_LAGRANGE_TETRAHEDRON },
  { "Lagrange Hexahedron", VTK_LAGRANGE_HEXAHEDRON }, { "Lagrange Wedge", VTK_LAGRANGE_WEDGE },
  { "Lagrange Pyramid", VTK_LAGRANGE_PYRAMID }, { "Bezier Curve", VTK_BEZIER_CURVE },
  { "Bezier Triangle", VTK_BEZIER_TRIANGLE }, { "Bezier Quadrilateral", VTK_BEZIER_QUADRILATERAL },
  { "Bezier Tetrahedron", VTK_BEZIER_TETRAHEDRON }, { "Bezier Hexahedron", VTK_BEZIER_HEXAHEDRON },
  { "Bezier Wedge", VTK_BEZIER_WEDGE }, { "Bezier Pyramid", VTK_BEZIER_PYRAMID } };
}

// ----------------------------------------------------------------------------
vtkPVExtractCellsByType::vtkPVExtractCellsByType()
{
  // Add cell types to selection (disabled by default)
  for (const auto& type : SupportedCellTypes)
  {
    this->CellTypeSelection->AddArray(type.first.c_str(), false);
  }

  // Add observer for selection update
  this->CellTypeSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkPVExtractCellsByType::Modified);
}

//------------------------------------------------------------------------------
int vtkPVExtractCellsByType::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Update selection of cell types
  this->RemoveAllCellTypes();

  for (const auto& type : SupportedCellTypes)
  {
    if (this->CellTypeSelection->ArrayIsEnabled(type.first.c_str()))
    {
      this->AddCellType(type.second);
    }
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}
