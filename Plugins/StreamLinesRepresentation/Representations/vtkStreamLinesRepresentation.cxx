/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkStreamLinesRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMergeBlocks.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamLinesMapper.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkTransform.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>
#include <map>
#include <string>

namespace
{
//----------------------------------------------------------------------------
void vtkGetNonGhostExtent(int* resultExtent, vtkImageData* dataSet)
{
  // this is really only meant for topologically structured grids
  dataSet->GetExtent(resultExtent);

  if (vtkUnsignedCharArray* ghostArray = vtkUnsignedCharArray::SafeDownCast(
        dataSet->GetCellData()->GetArray(vtkDataSetAttributes::GhostArrayName())))
  {
    // We have a ghost array. We need to iterate over the array to prune ghost
    // extents.

    int pntExtent[6];
    std::copy(resultExtent, resultExtent + 6, pntExtent);

    int validCellExtent[6];
    vtkStructuredData::GetCellExtentFromPointExtent(pntExtent, validCellExtent);

    // The start extent is the location of the first cell with ghost value 0.
    vtkIdType numTuples = ghostArray->GetNumberOfTuples();
    for (vtkIdType cc = 0; cc < numTuples; ++cc)
    {
      if (ghostArray->GetValue(cc) == 0)
      {
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoordsForExtent(cc, pntExtent, ijk);
        validCellExtent[0] = ijk[0];
        validCellExtent[2] = ijk[1];
        validCellExtent[4] = ijk[2];
        break;
      }
    }

    // The end extent is the  location of the last cell with ghost value 0.
    for (vtkIdType cc = (numTuples - 1); cc >= 0; --cc)
    {
      if (ghostArray->GetValue(cc) == 0)
      {
        int ijk[3];
        vtkStructuredData::ComputeCellStructuredCoordsForExtent(cc, pntExtent, ijk);
        validCellExtent[1] = ijk[0];
        validCellExtent[3] = ijk[1];
        validCellExtent[5] = ijk[2];
        break;
      }
    }

    // convert cell-extents to pt extents.
    resultExtent[0] = validCellExtent[0];
    resultExtent[2] = validCellExtent[2];
    resultExtent[4] = validCellExtent[4];

    resultExtent[1] = std::min(validCellExtent[1] + 1, resultExtent[1]);
    resultExtent[3] = std::min(validCellExtent[3] + 1, resultExtent[3]);
    resultExtent[5] = std::min(validCellExtent[5] + 1, resultExtent[5]);
  }
}
}

vtkStandardNewMacro(vtkStreamLinesRepresentation);

//----------------------------------------------------------------------------
vtkStreamLinesRepresentation::vtkStreamLinesRepresentation()
{
  this->StreamLinesMapper = vtkStreamLinesMapper::New();
  this->Property = vtkProperty::New();

  this->Actor = vtkPVLODActor::New();
  this->Actor->SetProperty(this->Property);
  this->Actor->SetEnableLOD(0);

  this->MBMerger = vtkMergeBlocks::New();

  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;
}

//----------------------------------------------------------------------------
vtkStreamLinesRepresentation::~vtkStreamLinesRepresentation()
{
  this->StreamLinesMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->MBMerger->Delete();
}

//----------------------------------------------------------------------------
int vtkStreamLinesRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamLinesRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->Cache);
    // BUG #14792.
    // We report this->DataSize explicitly since the data being "delivered" is
    // not the data that should be used to make rendering decisions based on
    // data size.
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds);

    // Pass partitioning information to the render view.
    vtkPVRenderView::SetOrderedCompositingConfiguration(
      inInfo, this, vtkPVRenderView::USE_BOUNDS_FOR_REDISTRIBUTION);

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    vtkPVRenderView::SetRequiresDistributedRenderingLOD(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    this->StreamLinesMapper->SetInputDataObject(vtkPVView::GetDeliveredPiece(inInfo, this));
    this->UpdateMapperParameters();
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkStreamLinesRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    vtkDataSet* inputDS = vtkDataSet::SafeDownCast(inputDO);
    vtkImageData* inputImage = vtkImageData::SafeDownCast(inputDS);
    vtkMultiBlockDataSet* inputMB = vtkMultiBlockDataSet::SafeDownCast(inputDO);
    if (inputImage)
    {
      vtkImageData* clone = inputImage->NewInstance();
      clone->ShallowCopy(inputImage);
      if (inputImage->HasAnyGhostCells())
      {
        int ext[6];
        vtkGetNonGhostExtent(ext, clone);
        // Yup, this will modify the "input", but that okay for now. Ultimately,
        // we will teach the volume mapper to handle ghost cells and this won't
        // be needed. Once that's done, we'll need to teach the KdTree
        // generation code to handle overlap in extents, however.
        clone->Crop(ext);
      }
      this->Cache.TakeReference(clone);
    }
    else if (inputDS)
    {
      this->Cache.TakeReference(inputDS->NewInstance());
      this->Cache->ShallowCopy(inputDS);
    }
    else if (inputMB)
    {
      this->MBMerger->SetInputDataObject(inputMB);
      this->MBMerger->Update();
      this->Cache.TakeReference(this->MBMerger->GetOutputDataObject(0)->NewInstance());
      this->Cache->ShallowCopy(this->MBMerger->GetOutputDataObject(0));
    }
    this->DataSize = this->Cache->GetActualMemorySize();
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkStreamLinesRepresentation::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    // Indicate that this is a prop to be rendered during hardware selection.
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkStreamLinesRepresentation::RemoveFromView(vtkView* view)
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
void vtkStreamLinesRepresentation::UpdateMapperParameters()
{
  this->Actor->SetMapper(this->StreamLinesMapper);
  this->Actor->SetVisibility(1);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//***************************************************************************
// Forwarded to Property.
//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetColor(double r, double g, double b)
{
  this->Property->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetLineWidth(double val)
{
  this->Property->SetLineWidth(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetOpacity(double val)
{
  this->Property->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetPointSize(double val)
{
  this->Property->SetPointSize(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetAmbientColor(double r, double g, double b)
{
  this->Property->SetAmbientColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetDiffuseColor(double r, double g, double b)
{
  this->Property->SetDiffuseColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetEdgeColor(double r, double g, double b)
{
  this->Property->SetEdgeColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetInterpolation(int val)
{
  this->Property->SetInterpolation(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetSpecularColor(double r, double g, double b)
{
  this->Property->SetSpecularColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//***************************************************************************
// Forwarded to Actor.
//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->Actor->SetVisibility(val ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetUserTransform(const double matrix[16])
{
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix);
  this->Actor->SetUserTransform(transform.GetPointer());
}

//***************************************************************************
// Forwarded to StreamLinesMapper.
//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetAnimate(bool val)
{
  this->StreamLinesMapper->SetAnimate(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetAlpha(double val)
{
  this->StreamLinesMapper->SetAlpha(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetStepLength(double val)
{
  this->StreamLinesMapper->SetStepLength(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetNumberOfParticles(int val)
{
  this->StreamLinesMapper->SetNumberOfParticles(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetMaxTimeToLive(int val)
{
  this->StreamLinesMapper->SetMaxTimeToLive(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetNumberOfAnimationSteps(int val)
{
  this->StreamLinesMapper->SetNumberOfAnimationSteps(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetInputVectors(
  int vtkNotUsed(idx), int port, int connection, int fieldAssociation, const char* name)
{
  this->StreamLinesMapper->SetInputArrayToProcess(1, port, connection, fieldAssociation, name);
}

//----------------------------------------------------------------------------
const char* vtkStreamLinesRepresentation::GetColorArrayName()
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    return info->Get(vtkDataObject::FIELD_NAME());
  }
  return NULL;
}

//****************************************************************************
// Methods merely forwarding parameters to internal objects.
//****************************************************************************

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->StreamLinesMapper->SetLookupTable(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetMapScalars(int val)
{
  if (val < 0 || val > 1)
  {
    vtkWarningMacro(<< "Invalid parameter for vtkStreamLinesRepresentation::SetMapScalars: "
                    << val);
    val = 0;
  }
  int mapToColorMode[] = { VTK_COLOR_MODE_DIRECT_SCALARS, VTK_COLOR_MODE_MAP_SCALARS };
  this->StreamLinesMapper->SetColorMode(mapToColorMode[val]);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetInterpolateScalarsBeforeMapping(int val)
{
  this->StreamLinesMapper->SetInterpolateScalarsBeforeMapping(val);
}

//----------------------------------------------------------------------------
void vtkStreamLinesRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  if (idx == 1)
  {
    return;
  }

  this->StreamLinesMapper->SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);

  if (name && name[0])
  {
    this->StreamLinesMapper->SetScalarVisibility(1);
    this->StreamLinesMapper->SelectColorArray(name);
    this->StreamLinesMapper->SetUseLookupTableScalarRange(1);
  }
  else
  {
    this->StreamLinesMapper->SetScalarVisibility(0);
    this->StreamLinesMapper->SelectColorArray(static_cast<const char*>(NULL));
  }

  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      this->StreamLinesMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    default:
      this->StreamLinesMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      break;
  }
}
