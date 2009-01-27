// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFluxGlyphs.cxx

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

#include "vtkFluxGlyphs.h"

#include "vtkArrowSource.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFluxVectors.h"
#include "vtkGeneralTransform.h"
#include "vtkGenericCell.h"
#include "vtkGlyph3D.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTransformFilter.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

//=============================================================================
// Computes the length of a 1D cell (only accurate for 1 segment cells).
inline double vtkFluxGlyphsCellLength(vtkCell *cell)
{
  vtkPoints *points = cell->GetPoints();
  double p0[3], p1[3];
  points->GetPoint(0, p0);
  points->GetPoint(cell->GetNumberOfPoints()-1, p1);
  return sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
}

// Computes the area of a 2D cell.
inline double vtkFluxGlyphsCellArea(vtkCell *cell)
{
  VTK_CREATE(vtkIdList, triangleIds);
  VTK_CREATE(vtkPoints, points);
  cell->Triangulate(0, triangleIds, points);
  int numTris = points->GetNumberOfPoints()/3;

  double totalArea = 0.0;
  for (int i = 0; i < numTris; i++)
    {
    double p0[3], p1[3], p2[3], v0[3], v1[3], vec[3];
    int j;
    points->GetPoint(i*3+0, p0);
    points->GetPoint(i*3+1, p1);
    points->GetPoint(i*3+2, p2);
    for (j = 0; j < 3; j++) v0[j] = p0[j] - p1[j];
    for (j = 0; j < 3; j++) v1[j] = p2[j] - p1[j];
    // The magnitude of the cross product is the area of the parallelogram of
    // the two vectors.  Half that is the area of the triangle.
    vtkMath::Cross(v1, v0, vec);
    totalArea += 0.5*vtkMath::Norm(vec);
    }

  return totalArea;
}

//=============================================================================
vtkCxxRevisionMacro(vtkFluxGlyphs, "1.2");
vtkStandardNewMacro(vtkFluxGlyphs);

//-----------------------------------------------------------------------------
vtkFluxGlyphs::vtkFluxGlyphs()
{
  this->SetInputArray(vtkDataSetAttributes::SCALARS);
}

vtkFluxGlyphs::~vtkFluxGlyphs()
{
}

void vtkFluxGlyphs::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkFluxGlyphs::SetInputArray(const char *name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               name);
}

void vtkFluxGlyphs::SetInputArray(int fieldAttributeType)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS,
                               fieldAttributeType);
}

//-----------------------------------------------------------------------------
int vtkFluxGlyphs::FillInputPortInformation(int port, vtkInformation *info)
{
  if (port == 0)
    {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkFluxGlyphs::RequestData(vtkInformation *vtkNotUsed(request),
                               vtkInformationVector **inputVector,
                               vtkInformationVector *outputVector)
{
  vtkDataSet *input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData *output = vtkPolyData::GetData(outputVector);

  if (!input || !output)
    {
    vtkErrorMacro(<< "Missing input or output?");
    return 0;
    }

  vtkSmartPointer<vtkDataSet> workingInput;
  workingInput.TakeReference(input->NewInstance());
  workingInput->ShallowCopy(input);

  vtkDataArray *inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (!inputArray)
    {
    vtkDebugMacro("No input scalars.");
    return 1;
    }

  // Find the scale factors and add them to the input.
  vtkSmartPointer<vtkDataArray> scaleFactors
    = this->MakeGlyphScaleFactors(workingInput);

  if (inputArray->GetNumberOfComponents() == 1)
    {
    workingInput = this->MakeFluxVectors(workingInput);
    }

  vtkSmartPointer<vtkPolyData> glyphs;
  glyphs = this->MakeGlyphs(workingInput, scaleFactors);

  output->ShallowCopy(glyphs);

  return 1;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDataSet> vtkFluxGlyphs::MakeFluxVectors(vtkDataSet *input)
{
  // Use a vtkFluxVectors filter to convert flux/circulation scalars to vectors.
  VTK_CREATE(vtkFluxVectors, makeVectors);
  makeVectors->SetInput(input);

  // Set the appropriate input array.
  vtkInformation *info = this->GetInputArrayInformation(0);
  if (info->Has(vtkDataObject::FIELD_NAME()))
    {
    makeVectors->SetInputArray(info->Get(vtkDataObject::FIELD_NAME()));
    }
  else
    {
    makeVectors->SetInputArray(info->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE()));
    }

  makeVectors->Update();

  vtkSmartPointer<vtkDataSet> result = makeVectors->GetOutput();
  return result;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray>
vtkFluxGlyphs::MakeGlyphScaleFactors(vtkDataSet *input)
{
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType cellId;

  VTK_CREATE(vtkDoubleArray, scaleFactors);
  scaleFactors->SetNumberOfComponents(1);
  scaleFactors->SetNumberOfTuples(numCells);

  vtkDataArray *inputArray = this->GetInputArrayToProcess(0, input);
  int numComponents = inputArray->GetNumberOfComponents();

  double maxFluxMag = 0.0;

  VTK_CREATE(vtkUnsignedCharArray, cellDims);
  cellDims->SetNumberOfComponents(1);
  cellDims->SetNumberOfTuples(numCells);
  double maxCellArea = 0.0;

  VTK_CREATE(vtkGenericCell, cell);
  for (cellId = 0; cellId < numCells; cellId++)
    {
    double fluxMag2 = 0.0;
    for (int c = 0; c < numComponents; c++)
      {
      double fluxComp = inputArray->GetComponent(cellId, c);
      fluxMag2 = fluxComp*fluxComp;
      }
    double fluxMag = sqrt(fluxMag2);
    if (fluxMag > maxFluxMag) maxFluxMag = fluxMag;

    input->GetCell(cellId, cell);
    int cDim = cell->GetCellDimension();
    cellDims->SetValue(cellId, cDim);
    double cellArea;
    switch (cDim)
      {
      case 1:
        cellArea = vtkFluxGlyphsCellLength(cell);
        break;
      case 2:
        cellArea = vtkFluxGlyphsCellArea(cell);
        if (maxCellArea < cellArea) maxCellArea = cellArea;
        break;
      default:
        // Should we warn?
        cellArea = 0.0;
        break;
      }

    scaleFactors->SetValue(cellId, fluxMag*cellArea);
    }

  // Normalize by flux magnitude and cell area.  The max length of the cell
  // should be roughly equal to the max length of a cell.  Use sqrt(maxCellArea)
  // to estimate that.
  if (maxFluxMag > 0.0)
    {
    double maxCellAreaSqrt = sqrt(maxCellArea);
    for (cellId = 0; cellId < numCells; cellId++)
      {
      double sf = scaleFactors->GetValue(cellId);
      sf /= maxFluxMag;
      if (cellDims->GetValue(cellId) == 2) sf /= maxCellAreaSqrt;
      scaleFactors->SetValue(cellId, sf);
      }
    }

  return scaleFactors;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkFluxGlyphs::MakeGlyphs(
                                                     vtkDataSet *input,
                                                     vtkDataArray *scaleFactors)
{
  // Add the scale factors to the input.
  scaleFactors->SetName("ScaleFactors");
  vtkSmartPointer<vtkDataSet> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->ShallowCopy(input);
  inputCopy->GetCellData()->AddArray(scaleFactors);

  // Find the parametric center of the cells.
  VTK_CREATE(vtkCellCenters, cellCenters);
  cellCenters->SetInput(inputCopy);

  // Create the glyph source.  Make the arrow shifted so that it is centered in
  // the cell.
  VTK_CREATE(vtkArrowSource, source);

  VTK_CREATE(vtkGeneralTransform, transform);
  transform->Translate(-0.5, 0.0, 0.0);
  VTK_CREATE(vtkTransformFilter, sourceTransform);
  sourceTransform->SetInputConnection(source->GetOutputPort());
  sourceTransform->SetTransform(transform);

  // Create the glyphs.
  VTK_CREATE(vtkGlyph3D, glyph);
  glyph->SetInputConnection(cellCenters->GetOutputPort());
  glyph->SetSourceConnection(sourceTransform->GetOutputPort());
  glyph->SetScaleFactor(1.0);
  glyph->OrientOn();
  glyph->SetScaleModeToScaleByScalar();
  glyph->SetInputArrayToProcess(0,0,0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                "ScaleFactors");

  // Set the appropriate vector to glyph on.
  vtkInformation *info = this->GetInputArrayInformation(0);
  if (info->Has(vtkDataObject::FIELD_NAME()))
    {
    glyph->SetInputArrayToProcess(1,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                  info->Get(vtkDataObject::FIELD_NAME()));
    }
  else
    {
    int attributeType = info->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
    if (attributeType == vtkDataSetAttributes::SCALARS)
      {
      attributeType = vtkDataSetAttributes::VECTORS;
      }
    glyph->SetInputArrayToProcess(1,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                  attributeType);
    }

  glyph->Update();

  vtkSmartPointer<vtkPolyData> result = glyph->GetOutput();
  // Modifying the output of a filter is not a great idea, but all we are
  // going to do is a shallow copy.
  result->GetPointData()->RemoveArray("ScaleFactors");
  result->GetPointData()->RemoveArray("GlyphVector");
  return result;
}

