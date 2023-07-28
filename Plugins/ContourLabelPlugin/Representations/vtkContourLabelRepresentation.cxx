// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkContourLabelRepresentation.h"
#include "vtkObjectFactory.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourFilter.h"
#include "vtkInformation.h"
#include "vtkInformationRequestKey.h"
#include "vtkInformationVector.h"
#include "vtkLabeledContourMapper.h"
#include "vtkMergeBlocks.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkPVView.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStripper.h"

vtkStandardNewMacro(vtkContourLabelRepresentation);

//----------------------------------------------------------------------------
vtkContourLabelRepresentation::vtkContourLabelRepresentation()
{
  this->Actor->SetMapper(this->Mapper);
  vtkMath::UninitializeBounds(this->VisibleDataBounds);
}

//------------------------------------------------------------------------------
vtkDataObject* vtkContourLabelRepresentation::GetRenderedDataObject(int vtkNotUsed(port))
{
  return this->Cache;
}

//------------------------------------------------------------------------------
int vtkContourLabelRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//------------------------------------------------------------------------------
int vtkContourLabelRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->Cache.Get());
    vtkPVRenderView::SetDeliverToAllProcesses(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
    {
      vtkAlgorithm* producer = producerPort->GetProducer();
      vtkPolyData* data =
        vtkPolyData::SafeDownCast(producer->GetOutputDataObject(producerPort->GetIndex()));
      if (data)
      {
        data->GetPointData()->SetActiveScalars(this->LabelArray.c_str());
      }
      this->Mapper->SetInputData(data);
      this->UpdateColoringParameters();
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkContourLabelRepresentation::UpdateColoringParameters()
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  bool supportColoring =
    info->Has(vtkDataObject::FIELD_ASSOCIATION()) && info->Has(vtkDataObject::FIELD_NAME());
  const int fieldAssoc = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  const char* fieldName = info->Get(vtkDataObject::FIELD_NAME());
  supportColoring = supportColoring && (fieldName && fieldName[0]);

  auto* polyMapper = this->Mapper->GetPolyDataMapper();
  if (supportColoring)
  {
    polyMapper->SetScalarVisibility(true);
    polyMapper->SelectColorArray(fieldName);
    polyMapper->SetUseLookupTableScalarRange(true);
    switch (fieldAssoc)
    {
      case vtkDataObject::FIELD_ASSOCIATION_CELLS:
        polyMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
        break;
      case vtkDataObject::FIELD_ASSOCIATION_NONE:
        polyMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
        polyMapper->SetFieldDataTupleId(0);
        break;
      case vtkDataObject::FIELD_ASSOCIATION_POINTS:
      default:
        polyMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    }
  }
  else
  {
    polyMapper->SetScalarVisibility(0);
    polyMapper->SelectColorArray(nullptr);
    polyMapper->SetUseLookupTableScalarRange(false);
  }
}

//------------------------------------------------------------------------------
void vtkContourLabelRepresentation::SetVisibility(bool value)
{
  this->Superclass::SetVisibility(value);
  this->Actor->SetVisibility(value);
}

//----------------------------------------------------------------------------
void vtkContourLabelRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* attributeTypeorName)
{
  if (idx == 0)
  {
    this->Superclass::SetInputArrayToProcess(
      idx, port, connection, fieldAssociation, attributeTypeorName);
  }
  else if (idx == 1 &&
    fieldAssociation == vtkDataObject::FieldAssociations::FIELD_ASSOCIATION_POINTS)
  {
    this->LabelArray = std::string(attributeTypeorName);
  }
}

//----------------------------------------------------------------------------
int vtkContourLabelRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPolyData> validDs;
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    validDs = vtkPolyData::SafeDownCast(input);
    auto* compositeInput = vtkCompositeDataSet::SafeDownCast(input);
    if (compositeInput != nullptr)
    {
      vtkNew<vtkMergeBlocks> merger;
      merger->SetMergePartitionsOnly(false);
      merger->SetOutputDataSetType(VTK_POLY_DATA);
      merger->SetTolerance(0.0);
      merger->SetMergePoints(false);
      merger->SetInputData(compositeInput);
      merger->Update();
      validDs = vtkPolyData::SafeDownCast(merger->GetOutput());
    }
  }

  if (validDs != nullptr)
  {
    vtkNew<vtkCellTypes> cellTypes;
    validDs->GetCellTypes(cellTypes);
    for (vtkIdType i = 0; i < cellTypes->GetNumberOfTypes(); ++i)
    {
      if (cellTypes->GetCellType(i) != VTK_LINE && cellTypes->GetCellType(i) != VTK_POLY_LINE)
      {
        validDs = nullptr;
        break;
      }
    }
  }

  if (validDs != nullptr)
  {
    validDs->GetBounds(this->VisibleDataBounds);

    vtkNew<vtkStripper> stripper;
    stripper->SetInputData(validDs);
    stripper->PassThroughPointIdsOff();
    stripper->PassThroughCellIdsOff();
    stripper->Update();

    this->Cache->ShallowCopy(stripper->GetOutput());
  }
  else
  {
    vtkWarningMacro("Labeled Contour: input needs to be a poly data of only lines or polylines.");
    vtkMath::UninitializeBounds(this->VisibleDataBounds);
    this->Cache->Initialize();
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//------------------------------------------------------------------------------
bool vtkContourLabelRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(view);
  }
  return false;
}

//------------------------------------------------------------------------------
bool vtkContourLabelRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkContourLabelRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void vtkContourLabelRepresentation::SetAmbientColor(double r, double g, double b)
{
  this->Actor->GetProperty()->SetAmbientColor(r, g, b);
}
void vtkContourLabelRepresentation::SetDiffuseColor(double r, double g, double b)
{
  this->Actor->GetProperty()->SetDiffuseColor(r, g, b);
}
void vtkContourLabelRepresentation::SetOpacity(float val)
{
  this->Actor->GetProperty()->SetOpacity(val);
}
void vtkContourLabelRepresentation::SetLineWidth(float val)
{
  this->Actor->GetProperty()->SetLineWidth(val);
}
void vtkContourLabelRepresentation::SetRenderLinesAsTubes(bool val)
{
  this->Actor->GetProperty()->SetRenderLinesAsTubes(val);
}
void vtkContourLabelRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->Mapper->SetLookupTable(val);
  this->Mapper->GetPolyDataMapper()->SetLookupTable(val);
}
void vtkContourLabelRepresentation::SetInterpolateScalarsBeforeMapping(bool val)
{
  this->Mapper->SetInterpolateScalarsBeforeMapping(val);
  this->Mapper->GetPolyDataMapper()->SetInterpolateScalarsBeforeMapping(val);
}
void vtkContourLabelRepresentation::SetMapScalars(int val)
{
  switch (val)
  {
    case 0:
      this->Mapper->SetColorModeToDirectScalars();
      this->Mapper->GetPolyDataMapper()->SetColorModeToDirectScalars();
      break;
    case 1:
      this->Mapper->SetColorModeToMapScalars();
      this->Mapper->GetPolyDataMapper()->SetColorModeToMapScalars();
      break;
    default:
      this->Mapper->SetColorModeToDefault();
      this->Mapper->GetPolyDataMapper()->SetColorModeToDefault();
  }
}
void vtkContourLabelRepresentation::SetLabelTextProperty(vtkTextProperty* prop)
{
  this->Mapper->SetTextProperty(prop);
}
void vtkContourLabelRepresentation::SetSkipDistance(float val)
{
  this->Mapper->SetSkipDistance(val);
}
