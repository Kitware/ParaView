/*=========================================================================

  Program:   ParaView
  Module:    vtkImageSliceRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageSliceRepresentation.h"

#include "vtkCommand.h"
#include "vtkExtractVOI.h"
#include "vtkImageData.h"
#include "vtkImageVolumeRepresentation.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVImageSliceMapper.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredExtent.h"

#include <algorithm>

vtkStandardNewMacro(vtkImageSliceRepresentation);
//----------------------------------------------------------------------------
vtkImageSliceRepresentation::vtkImageSliceRepresentation()
{
  this->Slice = 0;
  this->SliceMode = XY_PLANE;
  this->SliceMapper = vtkPVImageSliceMapper::New();
  this->Actor = vtkPVLODActor::New();
  this->Actor->SetMapper(this->SliceMapper);
  this->Actor->GetProperty()->LightingOff();
}

//----------------------------------------------------------------------------
vtkImageSliceRepresentation::~vtkImageSliceRepresentation()
{
  this->SliceMapper->SetInputData(0);
  this->SliceMapper->Delete();
  this->Actor->Delete();
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetSliceMode(int mode)
{
  if (this->SliceMode != mode)
  {
    this->SliceMode = mode;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetSlice(unsigned int val)
{
  if (this->Slice != val)
  {
    this->Slice = val;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetInputArrayToProcess(
  int idx, int port, int connection, int fieldAssociation, const char* name)
{
  this->Superclass::SetInputArrayToProcess(idx, port, connection, fieldAssociation, name);
  this->SliceMapper->SelectColorArray(name);
  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      this->SliceMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      this->SliceMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
      // Color entire block by zeroth tuple in the field data
      this->SliceMapper->SetFieldDataTupleId(0);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    default:
      this->SliceMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      break;
  }
}

//----------------------------------------------------------------------------
int vtkImageSliceRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageSliceRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // provide the "geometry" to the view so the view can delivery it to the
    // rendering nodes as and when needed.
    vtkPVView::SetPiece(inInfo, this, this->SliceData);
    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->SliceData->GetBounds());

    // BUG #14253: support translucent rendering.
    if (this->Actor->HasTranslucentPolygonalGeometry())
    {
      outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
      // Pass on the partitioning information to the view. This logic is similar
      // to what we do in vtkImageVolumeRepresentation.
      vtkPVRenderView::SetOrderedCompositingConfiguration(
        inInfo, this, vtkPVRenderView::USE_BOUNDS_FOR_REDISTRIBUTION, this->WholeBounds);
    }
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    // since there's no direct connection between the mapper and the collector,
    // we don't put an update-suppressor in the pipeline.
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
    {
      this->SliceMapper->SetInputConnection(producerPort);
      this->Actor->GetProperty()->ShadingOff();
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageSliceRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->WholeBounds);
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    this->UpdateSliceData(inputVector);
  }
  else
  {
    this->SliceData->Initialize();
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::UpdateSliceData(vtkInformationVector** inputVector)
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);

  input->GetBounds(this->WholeBounds);

  int inWholeExtent[6], outExt[6];
  memset(outExt, 0, sizeof(int) * 6);

  inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), inWholeExtent);
  int dataDescription = vtkStructuredData::SetExtent(inWholeExtent, outExt);

  if (vtkStructuredData::GetDataDimension(dataDescription) != 3)
  {
    this->SliceData->ShallowCopy(input);
    return;
  }

  int dims[3];
  dims[0] = inWholeExtent[1] - inWholeExtent[0] + 1;
  dims[1] = inWholeExtent[3] - inWholeExtent[2] + 1;
  dims[2] = inWholeExtent[5] - inWholeExtent[4] + 1;

  unsigned int slice = this->Slice;
  switch (this->SliceMode)
  {
    case YZ_PLANE:
      slice = (static_cast<int>(slice) >= dims[0]) ? dims[0] - 1 : slice;
      outExt[0] = outExt[1] = outExt[0] + slice;
      break;

    case XZ_PLANE:
      slice = (static_cast<int>(slice) >= dims[1]) ? dims[1] - 1 : slice;
      outExt[2] = outExt[3] = outExt[2] + slice;
      break;

    case XY_PLANE:
    default:
      slice = (static_cast<int>(slice) >= dims[2]) ? dims[2] - 1 : slice;
      outExt[4] = outExt[5] = outExt[4] + slice;
      break;
  }

  // Now, clamp the extent for the slice to the extent available on this rank.
  vtkNew<vtkStructuredExtent> helper;
  helper->Clamp(outExt, input->GetExtent());
  if (outExt[0] <= outExt[1] && outExt[2] <= outExt[3] && outExt[4] <= outExt[5])
  {
    vtkExtractVOI* voi = vtkExtractVOI::New();
    voi->SetVOI(outExt);
    voi->SetInputData(input);
    voi->Update();

    this->SliceData->ShallowCopy(voi->GetOutput());
    voi->Delete();
  }
  else
  {
    this->SliceData->Initialize();
  }

  // vtkExtractVOI is passing the origin for the individual slice.
  // We want to use the input origin/spacing to compute the bounds.
  this->SliceData->SetOrigin(input->GetOrigin());
}

//----------------------------------------------------------------------------
bool vtkImageSliceRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(rview);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkImageSliceRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return this->Superclass::RemoveFromView(rview);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//****************************************************************************
// Calls forwarded to internal objects.

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val ? 1 : 0);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetOpacity(double val)
{
  this->Actor->GetProperty()->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->SliceMapper->SetLookupTable(val);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetMapScalars(int val)
{
  this->SliceMapper->SetColorMode(val ? VTK_COLOR_MODE_MAP_SCALARS : VTK_COLOR_MODE_DIRECT_SCALARS);
}

//----------------------------------------------------------------------------
void vtkImageSliceRepresentation::SetUseXYPlane(int val)
{
  this->SliceMapper->SetUseXYPlane(val);
}
