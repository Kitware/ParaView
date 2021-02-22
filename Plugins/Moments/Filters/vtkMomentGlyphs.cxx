// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMomentGlyphs.cxx

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

#include "vtkMomentGlyphs.h"

#include "vtkArrowSource.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkGeneralTransform.h"
#include "vtkGenericCell.h"
#include "vtkGlyph3D.h"
#include "vtkInformation.h"
#include "vtkMath.h"
#include "vtkMomentVectors.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTransformFilter.h"
#include "vtkUnsignedCharArray.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <math.h>

//=============================================================================
vtkStandardNewMacro(vtkMomentGlyphs);

//-----------------------------------------------------------------------------
vtkMomentGlyphs::vtkMomentGlyphs()
{
  this->SetInputMoment(vtkDataSetAttributes::SCALARS);
  this->InputMomentIsDensity = 0;
  this->ScaleByDensity = 0;
}

vtkMomentGlyphs::~vtkMomentGlyphs() = default;

void vtkMomentGlyphs::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "InputMomentIsDensity: " << this->InputMomentIsDensity << endl;
  os << indent << "ScaleByDensity: " << this->ScaleByDensity << endl;
}

//-----------------------------------------------------------------------------
void vtkMomentGlyphs::SetInputMoment(const char* name)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, name);
}

void vtkMomentGlyphs::SetInputMoment(int fieldAttributeType)
{
  this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldAttributeType);
}

//-----------------------------------------------------------------------------
int vtkMomentGlyphs::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
int vtkMomentGlyphs::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  if (!input || !output)
  {
    vtkErrorMacro(<< "Missing input or output?");
    return 0;
  }

  vtkSmartPointer<vtkDataSet> workingInput;
  workingInput.TakeReference(input->NewInstance());
  workingInput->ShallowCopy(input);

  vtkSmartPointer<vtkDataArray> inputArray = this->GetInputArrayToProcess(0, inputVector);
  if (!inputArray)
  {
    vtkDebugMacro("No input scalars.");
    return 1;
  }
  if (!inputArray->GetName())
  {
    vtkErrorMacro("Input array needs a name.");
    return 1;
  }

  if (inputArray->GetNumberOfComponents() == 1)
  {
    this->MakeMomentVectors(workingInput, inputArray);
  }

  vtkSmartPointer<vtkPolyData> glyphs;
  glyphs = this->MakeGlyphs(workingInput, inputArray);

  output->ShallowCopy(glyphs);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkMomentGlyphs::MakeMomentVectors(
  vtkSmartPointer<vtkDataSet>& input, vtkSmartPointer<vtkDataArray>& inputArray)
{
  // Use a vtkMomentVectors filter to convert flux/circulation scalars to
  // vectors.
  VTK_CREATE(vtkMomentVectors, makeVectors);
  makeVectors->SetInputData(input);
  makeVectors->SetInputMoment(inputArray->GetName());
  makeVectors->SetInputMomentIsDensity(this->InputMomentIsDensity);

  makeVectors->Update();

  input = makeVectors->GetOutput();
  const char* inputArrayName;
  if (this->ScaleByDensity)
  {
    inputArrayName = makeVectors->GetOutputMomentDensityName();
  }
  else
  {
    inputArrayName = makeVectors->GetOutputMomentTotalName();
  }
  inputArray = input->GetCellData()->GetArray(inputArrayName);
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkMomentGlyphs::MakeGlyphScaleFactors(
  vtkDataSet* input, vtkDataArray* inputArray)
{
  vtkIdType numCells = input->GetNumberOfCells();
  vtkIdType cellId;

  VTK_CREATE(vtkDoubleArray, scaleFactors);
  scaleFactors->SetNumberOfComponents(1);
  scaleFactors->SetNumberOfTuples(numCells);

  int numComponents = inputArray->GetNumberOfComponents();

  double fitScaleFactor = VTK_DOUBLE_MAX;

  VTK_CREATE(vtkGenericCell, cell);
  for (cellId = 0; cellId < numCells; cellId++)
  {
    double fluxMag2 = 0.0;
    for (int c = 0; c < numComponents; c++)
    {
      double fluxComp = inputArray->GetComponent(cellId, c);
      fluxMag2 += fluxComp * fluxComp;
    }
    double fluxMag = sqrt(fluxMag2);
    scaleFactors->SetValue(cellId, fluxMag);

    // We want to scale the vectors so that they "fit" within the cell (the
    // length is roughly equal to the length of the cell).  Compute the scale
    // factor that will achieve that, and then record the overall scale that
    // will scale one such vector as this and constrain all others to be
    // no larger than that.
    if (fluxMag > 0.0)
    {
      input->GetCell(cellId, cell);
      double localFitScaleFactor = sqrt(cell->GetLength2()) / fluxMag;
      if (localFitScaleFactor < fitScaleFactor)
      {
        fitScaleFactor = localFitScaleFactor;
      }
    }
  }

  // Normalize the scale using the fitScaleFactor.
  for (cellId = 0; cellId < numCells; cellId++)
  {
    double sf = scaleFactors->GetValue(cellId);
    sf *= fitScaleFactor;
    scaleFactors->SetValue(cellId, sf);
  }

  return scaleFactors;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> vtkMomentGlyphs::MakeGlyphs(
  vtkDataSet* input, vtkDataArray* inputArray)
{
  // Add the scale factors to the input.
  vtkSmartPointer<vtkDataArray> scaleFactors = this->MakeGlyphScaleFactors(input, inputArray);
  scaleFactors->SetName("ScaleFactors");
  vtkSmartPointer<vtkDataSet> inputCopy;
  inputCopy.TakeReference(input->NewInstance());
  inputCopy->ShallowCopy(input);
  inputCopy->GetCellData()->AddArray(scaleFactors);

  // Find the parametric center of the cells.
  VTK_CREATE(vtkCellCenters, cellCenters);
  cellCenters->SetInputData(inputCopy);

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
  glyph->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "ScaleFactors");

  // Set the appropriate vector to glyph on.
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info->Has(vtkDataObject::FIELD_NAME()))
  {
    glyph->SetInputArrayToProcess(
      1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, info->Get(vtkDataObject::FIELD_NAME()));
  }
  else
  {
    int attributeType = info->Get(vtkDataObject::FIELD_ATTRIBUTE_TYPE());
    if (attributeType == vtkDataSetAttributes::SCALARS)
    {
      attributeType = vtkDataSetAttributes::VECTORS;
    }
    glyph->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, attributeType);
  }

  glyph->Update();

  vtkSmartPointer<vtkPolyData> result = glyph->GetOutput();
  // Modifying the output of a filter is not a great idea, but all we are
  // going to do is a shallow copy.
  result->GetPointData()->RemoveArray("ScaleFactors");
  result->GetPointData()->RemoveArray("GlyphVector");
  return result;
}
