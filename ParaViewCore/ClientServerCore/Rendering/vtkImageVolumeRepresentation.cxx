/*=========================================================================

  Program:   ParaView
  Module:    vtkImageVolumeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageVolumeRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkContourValues.h"
#include "vtkExtentTranslator.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPExtentTranslator.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStructuredData.h"
#include "vtkVolumeProperty.h"

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
    for (vtkIdType cc = 0, numTuples = ghostArray->GetNumberOfTuples(); cc < numTuples; ++cc)
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
    for (vtkIdType cc = (ghostArray->GetNumberOfTuples() - 1); cc >= 0; --cc)
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

vtkStandardNewMacro(vtkImageVolumeRepresentation);
//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::vtkImageVolumeRepresentation()
{
  this->VolumeMapper = vtkSmartVolumeMapper::New();
  this->Property = vtkVolumeProperty::New();

  this->Actor = vtkPVLODVolume::New();
  this->Actor->SetProperty(this->Property);

  this->OutlineSource = vtkOutlineSource::New();
  this->OutlineMapper = vtkPolyDataMapper::New();

  this->Cache = vtkImageData::New();
  this->Actor->SetLODMapper(this->OutlineMapper);

  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;

  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  this->MapScalars = true;
  this->MultiComponentsMapping = false;
}

//----------------------------------------------------------------------------
vtkImageVolumeRepresentation::~vtkImageVolumeRepresentation()
{
  this->VolumeMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->OutlineSource->Delete();
  this->OutlineMapper->Delete();

  this->Cache->Delete();
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // pass the actual volumetric data.
    vtkPVRenderView::SetPiece(inInfo, this, this->Cache, this->DataSize, 0);

    // pass the outline data, used on ranks where the data may not be available.
    vtkPVRenderView::SetPiece(inInfo, this, this->OutlineSource->GetOutputDataObject(0), 0, 1);

    // BUG #14792.
    // We report this->DataSize explicitly since the data being "delivered" is
    // not the data that should be used to make rendering decisions based on
    // data size.
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds);

    // Pass partitioning information to the render view.
    vtkPVRenderView::SetOrderedCompositingInformation(inInfo, this,
      this->PExtentTranslator.GetPointer(), this->WholeExtent, this->Origin, this->Spacing);

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    vtkPVRenderView::SetRequiresDistributedRenderingLOD(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto volumeProducer = vtkPVRenderView::GetPieceProducer(inInfo, this, 0);
    this->VolumeMapper->SetInputConnection(volumeProducer);
    this->UpdateMapperParameters();

    vtkAlgorithmOutput* outlineProducer = vtkPVRenderView::GetPieceProducer(inInfo, this, 1);
    this->OutlineMapper->SetInputConnection(outlineProducer);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageVolumeRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkImageData* input = vtkImageData::GetData(inputVector[0], 0);
    this->Cache->ShallowCopy(input);
    if (input->HasAnyGhostCells())
    {
      int ext[6];
      vtkGetNonGhostExtent(ext, this->Cache);
      // Yup, this will modify the "input", but that okay for now. Ultimately,
      // we will teach the volume mapper to handle ghost cells and this won't
      // be needed. Once that's done, we'll need to teach the KdTree
      // generation code to handle overlap in extents, however.
      this->Cache->Crop(ext);
    }

    this->Actor->SetEnableLOD(0);
    this->VolumeMapper->SetInputData(this->Cache);

    this->OutlineSource->SetBounds(this->Cache->GetBounds());
    this->OutlineSource->GetBounds(this->DataBounds);
    this->OutlineSource->Update();

    this->DataSize = this->Cache->GetActualMemorySize();

    // Collect information about volume that is needed for data redistribution
    // later.
    this->PExtentTranslator->GatherExtents(this->Cache);
    this->Cache->GetOrigin(this->Origin);
    this->Cache->GetSpacing(this->Spacing);
    vtkStreamingDemandDrivenPipeline::GetWholeExtent(
      inputVector[0]->GetInformationObject(0), this->WholeExtent);
  }
  else
  {
    // when no input is present, it implies that this processes is on a node
    // without the data input i.e. either client or render-server, in which case
    // we show only the outline.
    this->VolumeMapper->RemoveAllInputs();
    this->Actor->SetEnableLOD(1);
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    // Indicate that this is a prop to be rendered during hardware selection.
    rview->RegisterPropForHardwareSelection(this, this->GetRenderedProp());

    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkImageVolumeRepresentation::RemoveFromView(vtkView* view)
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
void vtkImageVolumeRepresentation::UpdateMapperParameters()
{
  const char* colorArrayName = NULL;
  int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
  }

  this->VolumeMapper->SelectScalarArray(colorArrayName);
  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
    default:
      this->VolumeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      break;
  }

  this->Actor->SetMapper(this->VolumeMapper);
  // this is necessary since volume mappers don't like empty arrays.
  this->Actor->SetVisibility(colorArrayName != NULL && colorArrayName[0] != 0);

  if (this->VolumeMapper->GetCropping())
  {
    double planes[6];
    for (int i = 0; i < 6; i++)
    {
      planes[i] = this->CroppingOrigin[i / 2] + this->DataBounds[i] * this->CroppingScale[i / 2];
    }
    this->VolumeMapper->SetCroppingRegionPlanes(planes);
  }

  if (this->Property)
  {
    if (this->MapScalars)
    {
      if (this->MultiComponentsMapping)
      {
        this->Property->SetIndependentComponents(false);
      }
      else
      {
        this->Property->SetIndependentComponents(true);
      }
    }
    else
    {
      this->Property->SetIndependentComponents(false);
    }

    // Update the mapper's vector mode
    vtkColorTransferFunction* ctf = this->Property->GetRGBTransferFunction(0);

    // When vtkScalarsToColors::MAGNITUDE mode is active, vtkSmartVolumeMapper
    // uses an internally generated (single-component) dataset.  However,
    // unchecking MapScalars (e.g. IndependentComponents == 0) requires 2C or 4C
    // data. In that case, vtkScalarsToColors::COMPONENT is forced in order to
    // make vtkSmartVolumeMapper use the original multiple-component dataset.
    int const indep = this->Property->GetIndependentComponents();
    int const mode = indep ? ctf->GetVectorMode() : vtkScalarsToColors::COMPONENT;
    int const comp = indep ? ctf->GetVectorComponent() : 0;

    this->VolumeMapper->SetVectorMode(mode);
    this->VolumeMapper->SetVectorComponent(comp);
  }
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Cropping Origin: " << this->CroppingOrigin[0] << ", " << this->CroppingOrigin[1]
     << ", " << this->CroppingOrigin[2] << endl;
  os << indent << "Cropping Scale: " << this->CroppingScale[0] << ", " << this->CroppingScale[1]
     << ", " << this->CroppingScale[2] << endl;
}

//***************************************************************************
// Forwarded to Actor.

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->Actor->SetVisibility(val ? 1 : 0);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetAmbient(double val)
{
  this->Property->SetAmbient(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetDiffuse(double val)
{
  this->Property->SetDiffuse(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSpecular(double val)
{
  this->Property->SetSpecular(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetShade(bool val)
{
  this->Property->SetShade(val);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetMapScalars(bool val)
{
  this->MapScalars = val;
  // the value is passed on to the vtkVolumeProperty in UpdateMapperParameters
  // since SetMapScalars and SetMultiComponentsMapping both control the same
  // vtkVolumeProperty ivar i.e. IndependentComponents.
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetMultiComponentsMapping(bool val)
{
  this->MultiComponentsMapping = val;
  // the value is passed on to the vtkVolumeProperty in UpdateMapperParameters
  // since SetMapScalars and SetMultiComponentsMapping both control the same
  // vtkVolumeProperty ivar i.e. IndependentComponents.
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetRequestedRenderMode(int mode)
{
  this->VolumeMapper->SetRequestedRenderMode(mode);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetShowIsosurfaces(int show)
{
  this->VolumeMapper->SetBlendMode(
    show ? vtkVolumeMapper::ISOSURFACE_BLEND : vtkVolumeMapper::COMPOSITE_BLEND);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetCropping(int crop)
{
  this->VolumeMapper->SetCropping(crop != 0);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetIsosurfaceValue(int i, double value)
{
  this->Property->GetIsoSurfaceValues()->SetValue(i, value);
}

//----------------------------------------------------------------------------
void vtkImageVolumeRepresentation::SetNumberOfIsosurfaces(int number)
{
  this->Property->GetIsoSurfaceValues()->SetNumberOfContours(number);
}
