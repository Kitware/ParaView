/*=========================================================================

  Program:   ParaView
  Module:    vtkUnstructuredGridVolumeRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkUnstructuredGridVolumeRepresentation.h"

#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper.h"
#include "vtkProjectedTetrahedraMapper.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRepresentationPreprocessor.h"

#include <map>
#include <string>

class vtkUnstructuredGridVolumeRepresentation::vtkInternals
{
public:
  typedef std::map<std::string,
          vtkSmartPointer<vtkUnstructuredGridVolumeMapper> > MapOfMappers;
  MapOfMappers Mappers;
  std::string ActiveVolumeMapper;
};


vtkStandardNewMacro(vtkUnstructuredGridVolumeRepresentation);
//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeRepresentation::vtkUnstructuredGridVolumeRepresentation()
{
  this->Internals = new vtkInternals();

  this->Preprocessor = vtkVolumeRepresentationPreprocessor::New();
  this->Preprocessor->SetTetrahedraOnly(1);

  this->CacheKeeper = vtkPVCacheKeeper::New();

  this->DefaultMapper = vtkProjectedTetrahedraMapper::New();
  this->Property = vtkVolumeProperty::New();
  this->Actor = vtkPVLODVolume::New();

  this->LODGeometryFilter = vtkPVGeometryFilter::New();
  this->LODGeometryFilter->SetUseOutline(0);

  this->LODMapper = vtkPolyDataMapper::New();
  this->CacheKeeper->SetInputConnection(this->Preprocessor->GetOutputPort());

  this->LODGeometryFilter->SetInputConnection(this->CacheKeeper->GetOutputPort());

  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->DefaultMapper);
  this->Actor->SetLODMapper(this->LODMapper);

  this->ColorArrayName = 0;
  this->ColorAttributeType = POINT_DATA;

  vtkMath::UninitializeBounds(this->DataBounds);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeRepresentation::~vtkUnstructuredGridVolumeRepresentation()
{
  this->Preprocessor->Delete();
  this->CacheKeeper->Delete();
  this->DefaultMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();

  this->LODGeometryFilter->Delete();
  this->LODMapper->Delete();

  this->SetColorArrayName(0);

  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::AddVolumeMapper(
  const char* name, vtkUnstructuredGridVolumeMapper* mapper)
{
  this->Internals->Mappers[name] = mapper;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetActiveVolumeMapper(
  const char* mapper)
{
  this->Internals->ActiveVolumeMapper = mapper? mapper : "";
  this->MarkModified();
}

//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeMapper*
vtkUnstructuredGridVolumeRepresentation::GetActiveVolumeMapper()
{
  if (this->Internals->ActiveVolumeMapper != "")
    {
    vtkInternals::MapOfMappers::iterator iter =
      this->Internals->Mappers.find(this->Internals->ActiveVolumeMapper);
    if (iter != this->Internals->Mappers.end() && iter->second.GetPointer())
      {
      return iter->second.GetPointer();
      }
    }

  return this->DefaultMapper;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::MarkModified()
{
  if (!this->GetUseCache())
    {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
    }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::FillInputPortInformation(
  int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    this->Preprocessor->SetInputConnection(
      this->GetInternalOutputPort());

    this->Preprocessor->Update();
    this->CacheKeeper->Update();

    vtkDataSet* ds = vtkDataSet::SafeDownCast(
      this->CacheKeeper->GetOutputDataObject(0));
    if (ds)
      {
      ds->GetBounds(this->DataBounds);
      }
    }
  else
    {
    this->Preprocessor->RemoveAllInputs();
    vtkNew<vtkUnstructuredGrid> placeholder;
    this->Preprocessor->SetInputData(0, placeholder.GetPointer());
    this->CacheKeeper->Update();
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkUnstructuredGridVolumeRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
    {
    return 0;
    }

  if (request_type == vtkPVView::REQUEST_UPDATE())
    {
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetPiece(inInfo, this,
      this->CacheKeeper->GetOutputDataObject(0));
    vtkPVRenderView::MarkAsRedistributable(inInfo, this);

    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this->DataBounds,
      matrix.GetPointer());
    }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
    {
    this->LODGeometryFilter->SetUseOutline(
      inInfo->Has(vtkPVRenderView::USE_OUTLINE_FOR_LOD())? 1 : 0);
    this->LODGeometryFilter->Update();
    vtkPVRenderView::SetPieceLOD(inInfo, this,
      this->LODGeometryFilter->GetOutputDataObject(0));
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    if (inInfo->Has(vtkPVRenderView::USE_LOD()))
      {
      this->Actor->SetEnableLOD(1);
      }
    else
      {
      this->Actor->SetEnableLOD(0);
      }

    vtkAlgorithmOutput* producerPort =
      vtkPVRenderView::GetPieceProducer(inInfo, this);
    vtkAlgorithmOutput* producerPortLOD =
      vtkPVRenderView::GetPieceProducerLOD(inInfo, this);

    vtkUnstructuredGridVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
    activeMapper->SetInputConnection(producerPort);
    this->LODMapper->SetInputConnection(producerPortLOD);

    this->UpdateMapperParameters();
    }

  return this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
}

//----------------------------------------------------------------------------
bool vtkUnstructuredGridVolumeRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkUnstructuredGridVolumeRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->RemoveActor(this->Actor);
    return true;
    }
  return false;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::UpdateMapperParameters()
{
  vtkUnstructuredGridVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
  activeMapper->SelectScalarArray(this->ColorArrayName);

  if (this->ColorArrayName && this->ColorArrayName[0])
    {
    this->LODMapper->SetScalarVisibility(1);
    this->LODMapper->SelectColorArray(this->ColorArrayName);
    }
  else
    {
    this->LODMapper->SetScalarVisibility(0);
    this->LODMapper->SelectColorArray(static_cast<const char*>(NULL));
    }

  switch (this->ColorAttributeType)
    {
  case CELL_DATA:
    activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
    break;

  case POINT_DATA:
  default:
    activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
    break;
    }

  this->Actor->SetMapper(activeMapper);
}


//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//***************************************************************************
// Forwarded to vtkVolumeRepresentationPreprocessor

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetExtractedBlockIndex(unsigned int index)
{
  this->Preprocessor->SetExtractedBlockIndex(index);
}

//***************************************************************************
// Forwarded to Actor.

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}
//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetPosition(double x , double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}
//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val? 1 : 0);
  this->Superclass::SetVisibility(val);
}

//***************************************************************************
// Forwarded to vtkVolumeProperty.
//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetInterpolationType(int val)
{
  this->Property->SetInterpolationType(val);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetColor(vtkColorTransferFunction* lut)
{
  this->Property->SetColor(lut);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetScalarOpacity(vtkPiecewiseFunction* pwf)
{
  this->Property->SetScalarOpacity(pwf);
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetScalarOpacityUnitDistance(double val)
{
  this->Property->SetScalarOpacityUnitDistance(val);
}
