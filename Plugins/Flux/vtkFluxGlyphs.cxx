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
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkTransformFilter.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

//=============================================================================
vtkCxxRevisionMacro(vtkFluxGlyphs, "1.1");
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
  if (inputArray->GetNumberOfComponents() == 1)
    {
    workingInput = this->MakeFluxVectors(workingInput);
    }

  vtkSmartPointer<vtkPolyData> glyphs;
  glyphs = this->MakeGlyphs(workingInput);

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

  VTK_CREATE(vtkDoubleArray, scaleFactors);
  scaleFactors->SetNumberOfComponents(1);
  scaleFactors->SetNumberOfTuples(numCells);

  VTK_CREATE(vtkGenericCell, cell);
  for (vtkIdType cellId = 0; cellId < numCells; cellId++)
    {
    input->GetCell(cellId, cell);
    scaleFactors->SetValue(cellId, sqrt(cell->GetLength2()));
    }

  return scaleFactors;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkFluxGlyphs::MakeGlyphs(vtkDataSet *input)
{
  // Find the scale factors and add them to the input.
  vtkSmartPointer<vtkDataArray> scaleFactors
    = this->MakeGlyphScaleFactors(input);
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
  return result;
}

