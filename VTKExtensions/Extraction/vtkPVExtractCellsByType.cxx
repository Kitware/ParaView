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
  { "Hexagonal Prism", VTK_HEXAGONAL_PRISM }, { "Polyhedron", VTK_POLYHEDRON } };
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
