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

#include "vtkAlgorithmOutput.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkDataSet.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPExtentTranslator.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkPolyDataMapper.h"
#include "vtkProjectedTetrahedraMapper.h"
#include "vtkRenderer.h"
#include "vtkResampleToImage.h"
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
  typedef std::map<std::string, vtkSmartPointer<vtkAbstractVolumeMapper> > MapOfMappers;
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

  this->ResampleToImageFilter = vtkResampleToImage::New();
  this->ResampleToImageFilter->SetSamplingDimensions(128, 128, 128);
  this->DataSize = 0;
  this->PExtentTranslator = vtkPExtentTranslator::New();
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0.0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0.0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;
  this->OutlineSource = vtkOutlineSource::New();

  this->DefaultMapper = vtkProjectedTetrahedraMapper::New();
  this->Property = vtkVolumeProperty::New();
  this->Actor = vtkPVLODVolume::New();

  this->LODGeometryFilter = vtkPVGeometryFilter::New();
  this->LODGeometryFilter->SetUseOutline(0);

  this->LODMapper = vtkPolyDataMapper::New();

  this->Actor->SetProperty(this->Property);
  this->Actor->SetMapper(this->DefaultMapper);
  this->Actor->SetLODMapper(this->LODMapper);
  vtkMath::UninitializeBounds(this->DataBounds);
  this->UseDataPartitions = false;
}

//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeRepresentation::~vtkUnstructuredGridVolumeRepresentation()
{
  this->Preprocessor->Delete();
  this->DefaultMapper->Delete();
  this->Property->Delete();
  this->Actor->Delete();

  this->ResampleToImageFilter->Delete();
  this->PExtentTranslator->Delete();
  this->OutlineSource->Delete();

  this->LODGeometryFilter->Delete();
  this->LODMapper->Delete();

  delete this->Internals;
  this->Internals = 0;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::AddVolumeMapper(
  const char* name, vtkAbstractVolumeMapper* mapper)
{
  this->Internals->Mappers[name] = mapper;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetActiveVolumeMapper(const char* mapper)
{
  this->Internals->ActiveVolumeMapper = mapper ? mapper : "";
  this->MarkModified();
}

//----------------------------------------------------------------------------
vtkAbstractVolumeMapper* vtkUnstructuredGridVolumeRepresentation::GetActiveVolumeMapper()
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
void vtkUnstructuredGridVolumeRepresentation::SetUseDataPartitions(bool val)
{
  if (this->UseDataPartitions != val)
  {
    this->UseDataPartitions = val;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (this->Internals->ActiveVolumeMapper == "Resample To Image")
  {
    return this->RequestDataResampleToImage(request, inputVector, outputVector);
  }

  vtkMath::UninitializeBounds(this->DataBounds);

  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    this->Preprocessor->SetInputConnection(this->GetInternalOutputPort());
    this->Preprocessor->Modified();
    this->Preprocessor->Update();
    vtkDataSet* ds = vtkDataSet::SafeDownCast(this->Preprocessor->GetOutputDataObject(0));
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
    this->Preprocessor->Update();
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (this->Internals->ActiveVolumeMapper == "Resample To Image")
  {
    return this->ProcessViewRequestResampleToImage(request_type, inInfo, outInfo);
  }

  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetPiece(inInfo, this, this->Preprocessor->GetOutputDataObject(0));

    if (this->UseDataPartitions == true)
    {
      // Pass partitioning information to the render view.
      vtkPVRenderView::SetOrderedCompositingInformation(inInfo, this->DataBounds);
    }
    else
    {
      vtkPVRenderView::MarkAsRedistributable(inInfo, this);
    }

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);

    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds, matrix.GetPointer());

    this->Actor->SetMapper(NULL);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    this->LODGeometryFilter->SetInputData(vtkPVView::GetPiece(inInfo, this));
    this->LODGeometryFilter->SetUseOutline(
      inInfo->Has(vtkPVRenderView::USE_OUTLINE_FOR_LOD()) ? 1 : 0);
    this->LODGeometryFilter->Update();
    vtkPVRenderView::SetPieceLOD(inInfo, this, this->LODGeometryFilter->GetOutputDataObject(0));
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    this->UpdateMapperParameters();
    if (inInfo->Has(vtkPVRenderView::USE_LOD()))
    {
      this->Actor->SetEnableLOD(1);
    }
    else
    {
      this->Actor->SetEnableLOD(0);
    }

    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    vtkAlgorithmOutput* producerPortLOD = vtkPVRenderView::GetPieceProducerLOD(inInfo, this);

    if (vtkUnstructuredGrid::SafeDownCast(producerPort->GetProducer()->GetOutputDataObject(0)))
    {
      vtkAbstractVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
      activeMapper->SetInputConnection(producerPort);
    }
    else
    {
      this->Actor->SetMapper(NULL);
    }
    this->LODMapper->SetInputConnection(producerPortLOD);
  }

  return 1;
}

//----------------------------------------------------------------------------
bool vtkUnstructuredGridVolumeRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);
    return this->Superclass::AddToView(view);
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
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::UpdateMapperParameters()
{
  vtkAbstractVolumeMapper* activeMapper = this->GetActiveVolumeMapper();
  const char* colorArrayName = NULL;
  int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;

  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    // The Resample To Image filter transforms cell data to point data.
    if (this->Internals->ActiveVolumeMapper == "Resample To Image")
    {
      fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    }
    else
    {
      fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
    }
  }

  activeMapper->SelectScalarArray(colorArrayName);
  if (colorArrayName && colorArrayName[0])
  {
    this->LODMapper->SetScalarVisibility(1);
    this->LODMapper->SelectColorArray(colorArrayName);
  }
  else
  {
    this->LODMapper->SetScalarVisibility(0);
    this->LODMapper->SelectColorArray(static_cast<const char*>(NULL));
  }

  switch (fieldAssociation)
  {
    case vtkDataObject::FIELD_ASSOCIATION_CELLS:
      activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_NONE:
      activeMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
      this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
      break;

    case vtkDataObject::FIELD_ASSOCIATION_POINTS:
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
  if (this->Preprocessor->GetExtractedBlockIndex() != index)
  {
    this->Preprocessor->SetExtractedBlockIndex(index);
    this->MarkModified();
  }
}

//***************************************************************************
// Forwarded to vtkResampleToImage

//----------------------------------------------------------------------------
void vtkUnstructuredGridVolumeRepresentation::SetSamplingDimensions(int xdim, int ydim, int zdim)
{
  this->ResampleToImageFilter->SetSamplingDimensions(xdim, ydim, zdim);
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
void vtkUnstructuredGridVolumeRepresentation::SetPosition(double x, double y, double z)
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
  this->Actor->SetVisibility(val ? 1 : 0);
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
  this->LODMapper->SetLookupTable(lut);
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

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::ProcessViewRequestResampleToImage(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }
  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    vtkPVRenderView::SetPiece(
      inInfo, this, this->OutlineSource->GetOutputDataObject(0), this->DataSize);
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);

    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds);

    // Pass partitioning information to the render view.
    vtkPVRenderView::SetOrderedCompositingInformation(
      inInfo, this, this->PExtentTranslator, this->WholeExtent, this->Origin, this->Spacing);

    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
    this->Actor->SetMapper(NULL);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    vtkPVRenderView::SetRequiresDistributedRenderingLOD(inInfo, this, true);
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    this->UpdateMapperParameters();

    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    if (producerPort)
    {
      this->LODMapper->SetInputConnection(producerPort);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkUnstructuredGridVolumeRepresentation::RequestDataResampleToImage(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkMath::UninitializeBounds(this->DataBounds);
  this->DataSize = 0;
  this->Origin[0] = this->Origin[1] = this->Origin[2] = 0;
  this->Spacing[0] = this->Spacing[1] = this->Spacing[2] = 0;
  this->WholeExtent[0] = this->WholeExtent[2] = this->WholeExtent[4] = 0;
  this->WholeExtent[1] = this->WholeExtent[3] = this->WholeExtent[5] = -1;

  vtkAbstractVolumeMapper* volumeMapper = this->GetActiveVolumeMapper();
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    this->ResampleToImageFilter->SetInputDataObject(input);
    this->ResampleToImageFilter->Update();

    this->Actor->SetEnableLOD(0);
    volumeMapper->SetInputConnection(this->ResampleToImageFilter->GetOutputPort());

    vtkImageData* output =
      vtkImageData::SafeDownCast(this->ResampleToImageFilter->GetOutputDataObject(0));
    this->OutlineSource->SetBounds(output->GetBounds());
    this->OutlineSource->GetBounds(this->DataBounds);
    this->OutlineSource->Update();

    this->DataSize = output->GetActualMemorySize();

    // Collect information about volume that is needed for data redistribution
    // later.
    this->PExtentTranslator->GatherExtents(output);
    output->GetOrigin(this->Origin);
    output->GetSpacing(this->Spacing);
    vtkStreamingDemandDrivenPipeline::GetWholeExtent(
      this->ResampleToImageFilter->GetOutputInformation(0), this->WholeExtent);
  }
  else
  {
    // when no input is present, it implies that this processes is on a node
    // without the data input i.e. either client or render-server, in which case
    // we show only the outline.
    volumeMapper->RemoveAllInputs();
    this->Actor->SetEnableLOD(1);
  }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}
