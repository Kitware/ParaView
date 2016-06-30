/*=========================================================================

  Program:   ParaView
  Module:    vtkCinemaLayerRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCinemaLayerRepresentation.h"

#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCinemaDatabase.h"
#include "vtkCinemaLayerMapper.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVCameraCollection.h"
#include "vtkPVRenderView.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"

#include <sstream>

vtkStandardNewMacro(vtkCinemaLayerRepresentation);
//----------------------------------------------------------------------------
vtkCinemaLayerRepresentation::vtkCinemaLayerRepresentation()
{
  vtkNew<vtkPolyData> pd;
  this->CacheKeeper->SetInputData(pd.Get());
  this->Actor->SetMapper(this->Mapper.Get());
  this->Actor->SetDisplayPosition(0, 0);
  this->Actor->SetWidth(1.0);
  this->Actor->SetHeight(1.0);
}

//----------------------------------------------------------------------------
vtkCinemaLayerRepresentation::~vtkCinemaLayerRepresentation()
{
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::SetVisibility(bool visibility)
{
  this->Superclass::SetVisibility(visibility);
  this->Actor->SetVisibility(visibility ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkCinemaLayerRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(inInfo, this, this->CacheKeeper->GetOutputDataObject(0));
    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
    if (vtkPVRenderView::GetDiscreteCameras(inInfo, this) == NULL)
    {
      vtkPVRenderView::SetDiscreteCameras(inInfo, this, this->Cameras.Get());
    }
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    this->UpdateMapper();
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkCinemaLayerRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::MarkModified()
{
  if (!this->GetUseCache())
  {
    this->CacheKeeper->RemoveAllCaches();
  }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkCinemaLayerRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
bool vtkCinemaLayerRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->AddPropToRenderer(this->Actor.Get());

    // Indicate that this is prop that we are rendering when hardware selection
    // is enabled.
    rview->RegisterPropForHardwareSelection(this, this->Actor.Get());
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkCinemaLayerRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->RemovePropFromRenderer(this->Actor.Get());
    rview->UnRegisterPropForHardwareSelection(this, this->Actor.Get());
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::SetLookupTable(vtkScalarsToColors* lut)
{
  this->Mapper->SetLookupTable(lut);
  this->MarkModified();
}

//----------------------------------------------------------------------------
int vtkCinemaLayerRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  this->CinemaDatabasePath = std::string();
  this->PipelineObject = std::string();
  this->BaseQueryJSON = std::string();
  this->CinemaTimeStep = std::string();
  this->FieldName = std::string();
  this->DefaultFieldName = std::string();
  this->Mapper->ClearLayers();
  this->Mapper->SetLayerProjectionMatrix(NULL);
  this->PreviousQueryJSON = std::string();
  this->Cameras->RemoveAllCameras();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
    this->CacheKeeper->SetInputData(input);
    this->CacheKeeper->Update();

    vtkStringArray* metaData = vtkStringArray::SafeDownCast(
      this->CacheKeeper->GetOutputDataObject(0)->GetFieldData()->GetAbstractArray(
        "CinemaDatabaseMetaData"));
    if (metaData && metaData->GetNumberOfTuples() == 4)
    {
      this->CinemaDatabasePath = metaData->GetValue(0);
      this->CinemaDatabase->Load(this->CinemaDatabasePath.c_str());
      this->PipelineObject = metaData->GetValue(1);
      this->BaseQueryJSON = metaData->GetValue(2);
      this->CinemaTimeStep = metaData->GetValue(3);
      this->FieldName = this->CinemaDatabase->GetFieldName(this->PipelineObject);

      // Now check what type of arrays can a layer provide
      const std::vector<std::string> values =
        this->CinemaDatabase->GetFieldValues(this->PipelineObject, "value");
      const std::vector<std::string> colors =
        this->CinemaDatabase->GetFieldValues(this->PipelineObject, "rgb");

      if (values.size() > 0)
      {
        this->DefaultFieldName = values[0];
      }
      else if (colors.size() > 0)
      {
        this->DefaultFieldName = colors[0];
      }

      // Update interactor style with available cameras
      const std::vector<vtkSmartPointer<vtkCamera> > cameras =
        this->CinemaDatabase->Cameras(this->CinemaTimeStep);
      for (size_t cc = 0; cc < cameras.size(); ++cc)
      {
        this->Cameras->AddCamera(cameras[cc]);
      }
    }
  }
  else
  {
    this->CacheKeeper->Update();
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::UpdateMapper()
{
  // First, build the query for the layers to render.
  std::ostringstream query;
  query << "{" << this->BaseQueryJSON;

  vtkSmartPointer<vtkCamera> layerCamera;

  // Try to get pose information from the view and add it to the query.
  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(this->GetView());
  vtkCamera* activeCamera = pvview ? pvview->GetActiveCamera() : NULL;
  if (activeCamera)
  {
    // FIXME: for now, I am just picking the closest camera. In reality, we need
    // to ensure that active camera as the located camera are compatible i.e.
    // have same position, fp, clipping range.
    int poseIndex = this->Cameras->FindClosestCamera(activeCamera);
    layerCamera = this->Cameras->GetCamera(poseIndex);
    query << ", 'pose' : " << poseIndex;
  }

  // Update query based on scalar coloring. If we're using scalar coloring, we
  // need to request appropriate layers.
  bool using_scalar_coloring = false;
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    const char* colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    // int fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
    if (colorArrayName && colorArrayName[0])
    {
      query << ", '" << this->FieldName.c_str() << "' : ['" << colorArrayName << "']";
      using_scalar_coloring = true;
    }
  }
  this->Mapper->SetScalarVisibility(using_scalar_coloring);

  // if not using scalar coloring, we still need to request a field for the
  // layer, so pick the default field, if provided.
  if (!using_scalar_coloring && !this->DefaultFieldName.empty())
  {
    query << ", '" << this->FieldName.c_str() << "' : ['" << this->DefaultFieldName.c_str() << "']";
  }

  query << "}";

  // Now check the query with previous one, if the query didn't change, we don't
  // need to fetch new layers. Eventually, we can also support for caching.
  if (query.str() == this->PreviousQueryJSON)
  {
    return;
  }

  this->PreviousQueryJSON = query.str();

  // Now, get the layers for this query from the cinema database.
  const std::vector<vtkSmartPointer<vtkImageData> > layers =
    this->CinemaDatabase->TranslateQuery(query.str());

  this->Mapper->SetLayers(layers);
  if (layers.size() > 0 && layerCamera)
  {
    int dims[3];
    layers[0]->GetDimensions(dims);
    this->Mapper->SetLayerProjectionMatrix(
      layerCamera->GetProjectionTransformMatrix(static_cast<double>(dims[0]) / dims[1], 0.0, 1.0));
    this->Mapper->SetLayerCameraViewUp(layerCamera->GetViewUp());
  }
  else
  {
    this->Mapper->SetLayerProjectionMatrix(NULL);
  }
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::SetRenderLayersAsImage(bool val)
{
  this->Mapper->SetRenderLayersAsImage(val);
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
