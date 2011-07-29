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
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPKdTree.h"
#include "vtkPolyDataMapper.h"
#include "vtkProjectedTetrahedraMapper.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredDataDeliveryFilter.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeRepresentationPreprocessor.h"

#include <vtkstd/map>
#include <vtkstd/string>

class vtkUnstructuredGridVolumeRepresentation::vtkInternals
{
public:
  typedef vtkstd::map<vtkstd::string,
          vtkSmartPointer<vtkUnstructuredGridVolumeMapper> > MapOfMappers;
  MapOfMappers Mappers;
  vtkstd::string ActiveVolumeMapper;
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
  this->DeliveryFilter = vtkUnstructuredDataDeliveryFilter::New();
  this->DeliveryFilter->SetOutputDataType(VTK_UNSTRUCTURED_GRID);

  this->DeliverySuppressor = vtkPVUpdateSuppressor::New();
  this->DeliverySuppressor->SetInputConnection(this->DeliveryFilter->GetOutputPort());

  this->Distributor = vtkOrderedCompositeDistributor::New();
  this->Distributor->SetController(vtkMultiProcessController::GetGlobalController());
  this->Distributor->SetInputConnection(this->DeliverySuppressor->GetOutputPort());

  this->UpdateSuppressor = vtkPVUpdateSuppressor::New();
  this->UpdateSuppressor->SetInputConnection(this->Distributor->GetOutputPort());

  this->LODGeometryFilter = vtkPVGeometryFilter::New();
  this->LODGeometryFilter->SetUseOutline(0);

  this->LODMapper = vtkPolyDataMapper::New();
  this->LODDeliveryFilter = vtkUnstructuredDataDeliveryFilter::New();
  this->LODDeliveryFilter->SetOutputDataType(VTK_POLY_DATA);
  this->LODDeliveryFilter->SetLODMode(true);

  this->LODDeliverySuppressor = vtkPVUpdateSuppressor::New();
  this->LODUpdateSuppressor = vtkPVUpdateSuppressor::New();

  this->CacheKeeper->SetInputConnection(this->Preprocessor->GetOutputPort());
  this->LODGeometryFilter->SetInputConnection(this->CacheKeeper->GetOutputPort());
  this->LODDeliverySuppressor->SetInputConnection(this->LODDeliveryFilter->GetOutputPort());
  this->LODUpdateSuppressor->SetInputConnection(this->LODDeliverySuppressor->GetOutputPort());
  this->LODMapper->SetInputConnection(
    this->LODUpdateSuppressor->GetOutputPort());
  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->DefaultMapper);
  this->Actor->SetLODMapper(this->LODMapper);

  this->ColorArrayName = 0;
  this->ColorAttributeType = POINT_DATA;
}

//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeRepresentation::~vtkUnstructuredGridVolumeRepresentation()
{
  this->Preprocessor->Delete();
  this->CacheKeeper->Delete();
  this->DefaultMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();
  this->DeliveryFilter->Delete();
  this->Distributor->Delete();
  this->UpdateSuppressor->Delete();
  this->LODUpdateSuppressor->Delete();

  this->LODGeometryFilter->Delete();
  this->LODMapper->Delete();
  this->LODDeliveryFilter->Delete();

  this->DeliverySuppressor->Delete();
  this->LODDeliverySuppressor->Delete();

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
  // mark delivery filters modified.
  this->DeliveryFilter->Modified();
  this->Distributor->Modified();
  this->LODDeliveryFilter->Modified();

  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    this->Preprocessor->SetInputConnection(
      this->GetInternalOutputPort());
    this->Preprocessor->Update();
    this->DeliveryFilter->SetInputConnection(
      this->CacheKeeper->GetOutputPort());
    this->LODDeliveryFilter->SetInputConnection(
      this->LODGeometryFilter->GetOutputPort());
    }
  else
    {
    this->Preprocessor->RemoveAllInputs();
    this->DeliveryFilter->RemoveAllInputs();
    this->LODDeliveryFilter->RemoveAllInputs();
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
  if (request_type == vtkPVView::REQUEST_INFORMATION())
    {
    vtkDataObject* geom = this->Preprocessor->GetOutputDataObject(0);
    if (geom)
      {
      outInfo->Set(vtkPVRenderView::GEOMETRY_SIZE(),
        geom->GetActualMemorySize());
      }
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    outInfo->Set(vtkPVRenderView::REDISTRIBUTABLE_DATA_PRODUCER(),
      this->DeliveryFilter);
    }
  else if (request_type == vtkPVView::REQUEST_PREPARE_FOR_RENDER())
    {
    // // In REQUEST_PREPARE_FOR_RENDER, we need to ensure all our data-deliver
    // // filters have their states updated as requested by the view.

    // // this is where we will look to see on what nodes are we going to render and
    // // render set that up.

    if (inInfo->Has(vtkPVRenderView::USE_LOD()))
      {
      this->LODDeliveryFilter->ProcessViewRequest(inInfo);
      this->Actor->SetEnableLOD(1);
      if (this->LODDeliverySuppressor->GetForcedUpdateTimeStamp() <
        this->LODDeliveryFilter->GetMTime())
        {
        outInfo->Set(vtkPVRenderView::NEEDS_DELIVERY(), 1);
        }
      }
    else
      {
      this->DeliveryFilter->ProcessViewRequest(inInfo);
      this->Actor->SetEnableLOD(0);
      if (this->DeliverySuppressor->GetForcedUpdateTimeStamp() <
        this->DeliveryFilter->GetMTime())
        {
        outInfo->Set(vtkPVRenderView::NEEDS_DELIVERY(), 1);
        }
      }
    }
  else if (request_type == vtkPVView::REQUEST_DELIVERY())
    {
    if (this->Actor->GetEnableLOD())
      {
      this->LODDeliveryFilter->Modified();
      this->LODDeliverySuppressor->ForceUpdate();
      }
    else
      {
      this->DeliveryFilter->Modified();
      this->DeliverySuppressor->ForceUpdate();
      }
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    // typically, representations don't do anything special in this pass.
    // However, when we are doing ordered compositing, we need to ensure that
    // the redistribution of data happens in this pass.
    if (inInfo->Has(vtkPVRenderView::KD_TREE()))
      {
      vtkPKdTree* kdTree = vtkPKdTree::SafeDownCast(
        inInfo->Get(vtkPVRenderView::KD_TREE()));
      this->Distributor->SetPKdTree(kdTree);
      this->Distributor->SetPassThrough(0);
      }
    else
      {
      this->Distributor->SetPKdTree(NULL);
      this->Distributor->SetPassThrough(1);
      }
    this->UpdateMapperParameters();
    if (!this->Actor->GetEnableLOD())
      {
      this->UpdateSuppressor->ForceUpdate();
      }
    else
      {
      this->LODUpdateSuppressor->ForceUpdate();
      }
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

  activeMapper->SetInputConnection(this->UpdateSuppressor->GetOutputPort());
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
