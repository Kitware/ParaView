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
#include "vtkImageMapper.h"
#include "vtkImageReslice.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPVCameraCollection.h"
#include "vtkPVRenderView.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkStringArray.h"
#include "vtkTransform.h"

#include <sstream>

vtkStandardNewMacro(vtkCinemaLayerRepresentation);
//----------------------------------------------------------------------------
vtkCinemaLayerRepresentation::vtkCinemaLayerRepresentation()
{
  this->Reslice->SetInputData(this->CachedImage);
  this->MapperA->SetInputConnection(this->Reslice->GetOutputPort());
  this->MapperA->SetColorWindow(255);
  this->MapperA->SetColorLevel(127.5);

  this->Actor->SetMapper(this->MapperA.Get());
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
    vtkPVRenderView::SetPiece(inInfo, this, this->Data);
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
  this->MapperC->SetLookupTable(lut);
  this->MarkModified();
}

//----------------------------------------------------------------------------
int vtkCinemaLayerRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->CinemaDatabasePath = std::string();
  this->PipelineObject = std::string();
  this->BaseQueryJSON = std::string();
  this->CinemaTimeStep = std::string();
  this->FieldName = std::string();
  this->DefaultFieldName = std::string();
  this->MapperC->ClearLayers();
  this->MapperC->SetLayerProjectionMatrix(NULL);
  this->PreviousQueryJSON = std::string();
  this->Cameras->RemoveAllCameras();

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkPolyData* input = vtkPolyData::GetData(inputVector[0], 0);
    this->Data->ShallowCopy(input);
    vtkStringArray* metaData = vtkStringArray::SafeDownCast(
      input->GetFieldData()->GetAbstractArray("CinemaDatabaseMetaData"));
    if (metaData && metaData->GetNumberOfTuples() == 4)
    {
      this->CinemaDatabasePath = metaData->GetValue(0);
      this->CinemaDatabase->Load(this->CinemaDatabasePath.c_str());
      this->PipelineObject = metaData->GetValue(1);
      this->BaseQueryJSON = metaData->GetValue(2);
      this->CinemaTimeStep = metaData->GetValue(3);
      this->FieldName = this->CinemaDatabase->GetFieldName(this->PipelineObject);

      if (this->CinemaDatabase->GetSpec() == vtkCinemaDatabase::CINEMA_SPEC_A)
      {
        this->Actor->SetMapper(this->MapperA.Get());
      }
      else if (this->CinemaDatabase->GetSpec() == vtkCinemaDatabase::CINEMA_SPEC_C)
      {
        this->Actor->SetMapper(this->MapperC.Get());
      }

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
    this->Data->Initialize();
  }
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::UpdateMapper()
{
  // First, create a query.
  std::ostringstream query;
  query << "{";

  // Base query contains parameters
  if (!this->BaseQueryJSON.empty())
  {
    query << this->BaseQueryJSON;
  }

  vtkPVRenderView* pvview = vtkPVRenderView::SafeDownCast(this->GetView());
  vtkCamera* activeCamera = pvview ? pvview->GetActiveCamera() : NULL;
  int cameraIndex = -1;
  vtkSmartPointer<vtkCamera> layerCamera;
  if (activeCamera)
  {
    // FIXME: for now, I am just picking the closest camera. In reality, we need
    // to ensure that active camera and the located camera are compatible i.e.
    // have same position, fp, clipping range.
    cameraIndex = this->Cameras->FindClosestCamera(activeCamera);
    layerCamera = this->Cameras->GetCamera(cameraIndex);
  }

  // Spec-dependent query contains camera info
  if (this->CinemaDatabase->GetSpec() == vtkCinemaDatabase::CINEMA_SPEC_A)
  {
    query << this->GetSpecAQuery(cameraIndex);
  }
  else if (this->CinemaDatabase->GetSpec() == vtkCinemaDatabase::CINEMA_SPEC_C)
  {
    query << this->GetSpecCQuery(cameraIndex);
  }

  query << "}";

  std::string queryString = query.str();
  size_t last = queryString.find_last_of(",");
  if (last != std::string::npos)
  {
    queryString.erase(last, 1);
  }

  // If the query didn't change, we don't need to fetch new layers.
  std::vector<vtkSmartPointer<vtkImageData> > layers;
  if (queryString != this->PreviousQueryJSON)
  {
    this->PreviousQueryJSON = queryString;
    layers = this->CinemaDatabase->TranslateQuery(queryString);
    if (layers.size() > 0)
    {
      // Cache first layer (i.e. full image for spec A, but not for spec C)
      this->CachedImage->DeepCopy(layers.at(0));
    }
  }
  vtkImageData* image = this->CachedImage.Get();

  if (this->CinemaDatabase->GetSpec() == vtkCinemaDatabase::CINEMA_SPEC_A && image != NULL &&
    layerCamera != NULL)
  {
    if (!this->RenderLayerAsImage)
    {
      // Get image center
      double bounds[6];
      image->GetBounds(bounds);
      double center[3];
      center[0] = (bounds[1] + bounds[0]) / 2.0;
      center[1] = (bounds[3] + bounds[2]) / 2.0;
      center[2] = (bounds[5] + bounds[4]) / 2.0;

      vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
      // manipulate relative to center of image
      transform->Translate(center[0], center[1], center[2]);
      // flip Y
      transform->Scale(1.0, -1.0, 1.0);
      // used to rotate about Z, here but that is problematic
      transform->Translate(-center[0], -center[1], -center[2]);

      // scale the image to fit the view
      vtkView* view = this->GetView();
      vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
      double* rendererCenter = rview->GetRenderer()->GetCenter();
      double scalex = center[0] / rendererCenter[0];
      double scaley = center[1] / rendererCenter[1];
      double scale = std::max(scalex, scaley);
      transform->Scale(scale, scale, 1);
      transform->Update();

      if (scalex > scaley)
      {
        transform->Translate(0, center[1] / scale - rendererCenter[1], 0);
      }
      else
      {
        transform->Translate(center[0] / scale - rendererCenter[0], 0, 0);
      }

      this->Reslice->SetResliceTransform(transform);

      double* backgroundColor = rview->GetRenderer()->GetBackground();
      // if loaded image has no opacity channel, vtkImageReslice will not consider alpha value.
      this->Reslice->SetBackgroundColor(
        255 * backgroundColor[0], 255 * backgroundColor[1], 255 * backgroundColor[2], 0);
    }
    this->Reslice->SetOutputSpacing(image->GetSpacing());
    this->Reslice->SetOutputOrigin(image->GetOrigin());
    this->Reslice->SetOutputExtent(image->GetExtent());
    this->Reslice->Update();
  }
  else if (this->CinemaDatabase->GetSpec() == vtkCinemaDatabase::CINEMA_SPEC_C)
  {
    if (layers.size() > 0)
    {
      this->MapperC->SetLayers(layers);
    }
    if (image != NULL && layerCamera)
    {
      int dims[3];
      image->GetDimensions(dims);
      this->MapperC->SetLayerProjectionMatrix(layerCamera->GetProjectionTransformMatrix(
        static_cast<double>(dims[0]) / dims[1], 0.0, 1.0));
      this->MapperC->SetLayerCameraViewUp(layerCamera->GetViewUp());
    }
    else
    {
      this->MapperC->SetLayerProjectionMatrix(NULL);
    }
  }
}

//----------------------------------------------------------------------------
std::string vtkCinemaLayerRepresentation::GetSpecCQuery(int cameraIndex)
{
  std::ostringstream query;

  if (cameraIndex != -1)
  {
    query << "'pose' : " << cameraIndex << ", ";
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
      query << "'" << this->FieldName.c_str() << "' : ['" << colorArrayName << "'],";
      using_scalar_coloring = true;
    }
  }
  this->MapperC->SetScalarVisibility(using_scalar_coloring);

  // if not using scalar coloring, we still need to request a field for the
  // layer, so pick the default field, if provided.
  if (!using_scalar_coloring && !this->DefaultFieldName.empty())
  {
    query << "'" << this->FieldName.c_str() << "' : ['" << this->DefaultFieldName.c_str() << "'],";
  }

  return query.str();
}

//----------------------------------------------------------------------------
std::string vtkCinemaLayerRepresentation::GetSpecAQuery(int cameraIndex)
{
  std::ostringstream query;

  if (cameraIndex != -1)
  {
    std::vector<std::string> phiValues = this->CinemaDatabase->GetControlParameterValues("phi");
    std::vector<std::string> thetaValues = this->CinemaDatabase->GetControlParameterValues("theta");

    if (phiValues.size() > 0)
    {
      size_t phiIndex = cameraIndex / (thetaValues.size() > 0 ? thetaValues.size() : 1);
      query << "'phi' : [" << phiValues[phiIndex] << "],";
    }

    if (thetaValues.size() > 0)
    {
      size_t thetaIndex = cameraIndex % thetaValues.size();
      query << "'theta' : [" << thetaValues[thetaIndex] << "],";
    }
  }

  return query.str();
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::SetRenderLayersAsImage(bool val)
{
  this->RenderLayerAsImage = val;
  this->MapperC->SetRenderLayersAsImage(val);
}

//----------------------------------------------------------------------------
void vtkCinemaLayerRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
