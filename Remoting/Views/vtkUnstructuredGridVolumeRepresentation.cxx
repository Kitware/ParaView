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
#include "vtkDataSetAttributes.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockVolumeMapper.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODVolume.h"
#include "vtkPVRenderView.h"
#include "vtkPolyDataMapper.h"
#include "vtkProjectedTetrahedraMapper.h"
#include "vtkRenderer.h"
#include "vtkResampleToImage.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkSmartVolumeMapper.h"
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
  : Superclass()
{
  this->Internals = new vtkInternals();

  this->Preprocessor->SetTetrahedraOnly(1);

  this->ResampleToImageFilter->SetSamplingDimensions(128, 128, 128);

  this->LODGeometryFilter->SetUseOutline(0);

  this->Actor->SetMapper(this->DefaultMapper);
  this->Actor->SetLODMapper(this->LODMapper);
}

//----------------------------------------------------------------------------
vtkUnstructuredGridVolumeRepresentation::~vtkUnstructuredGridVolumeRepresentation()
{
  delete this->Internals;
  this->Internals = nullptr;
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

    if (this->UseSeparateOpacityArray)
    {
      this->AppendOpacityComponent(ds);
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
      // We want to use this representation's data bounds to redistribute all other data in the
      // scene if ordered compositing is needed.
      vtkPVRenderView::SetOrderedCompositingConfiguration(
        inInfo, this, vtkPVRenderView::USE_BOUNDS_FOR_REDISTRIBUTION);
    }
    else
    {
      // We want to let vtkPVRenderView do redistribution of data as necessary,
      // and use this representations data for determining a load balanced distribution
      // if ordered is needed.
      vtkPVRenderView::SetOrderedCompositingConfiguration(inInfo, this,
        vtkPVRenderView::DATA_IS_REDISTRIBUTABLE | vtkPVRenderView::USE_DATA_FOR_LOAD_BALANCING);
    }
    vtkPVRenderView::SetRedistributionModeToUniquelyAssignBoundaryCells(inInfo, this);
    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);

    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->DataBounds, matrix.GetPointer());

    this->Actor->SetMapper(nullptr);
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
      this->Actor->SetMapper(nullptr);
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
  const char* colorArrayName = nullptr;
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

  if (this->UseSeparateOpacityArray)
  {
    // See AppendOpacityComponent() for the construction of this array.
    std::string combinedName(colorArrayName);
    combinedName += "_and_opacity";
    activeMapper->SelectScalarArray(combinedName.c_str());

    if (colorArrayName && colorArrayName[0])
    {
      this->LODMapper->SetScalarVisibility(1);
      this->LODMapper->SelectColorArray(combinedName.c_str());
    }
    else
    {
      this->LODMapper->SetScalarVisibility(0);
      this->LODMapper->SelectColorArray(static_cast<const char*>(nullptr));
    }
  }
  else
  {
    activeMapper->SelectScalarArray(colorArrayName);

    if (colorArrayName && colorArrayName[0])
    {
      this->LODMapper->SetScalarVisibility(1);
      this->LODMapper->SelectColorArray(colorArrayName);
    }
    else
    {
      this->LODMapper->SetScalarVisibility(0);
      this->LODMapper->SelectColorArray(static_cast<const char*>(nullptr));
    }
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

  if (this->Property)
  {
    if (this->MapScalars)
    {
      if (this->MultiComponentsMapping || this->UseSeparateOpacityArray)
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

    // Logic borrowed from vtkImageVolumeRepresentation::UpdateMapperParameters()
    int const indep = this->Property->GetIndependentComponents();
    int const mode = indep ? ctf->GetVectorMode() : vtkScalarsToColors::COMPONENT;
    int const comp = indep ? ctf->GetVectorComponent() : 0;

    if (auto smartVolumeMapper = vtkSmartVolumeMapper::SafeDownCast(activeMapper))
    {
      smartVolumeMapper->SetVectorMode(mode);
      smartVolumeMapper->SetVectorComponent(comp);
    }
    else if (auto mbMapper = vtkMultiBlockVolumeMapper::SafeDownCast(activeMapper))
    {
      mbMapper->SetVectorMode(mode);
      mbMapper->SetVectorComponent(comp);
    }
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
    vtkPVRenderView::SetOrderedCompositingConfiguration(
      inInfo, this, vtkPVRenderView::USE_BOUNDS_FOR_REDISTRIBUTION);
    vtkPVRenderView::SetRequiresDistributedRendering(inInfo, this, true);
    this->Actor->SetMapper(nullptr);
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

  vtkAbstractVolumeMapper* volumeMapper = this->GetActiveVolumeMapper();
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
    this->ResampleToImageFilter->SetInputDataObject(input);
    this->ResampleToImageFilter->Update();

    this->Actor->SetEnableLOD(0);

    auto resampleOutput = this->ResampleToImageFilter->GetOutput();
    vtkSmartPointer<vtkDataSet> ds;
    ds.TakeReference(resampleOutput->NewInstance());
    ds->ShallowCopy(resampleOutput);
    if (this->UseSeparateOpacityArray)
    {
      this->AppendOpacityComponent(ds);
    }

    volumeMapper->SetInputDataObject(ds);

    vtkImageData* output = vtkImageData::SafeDownCast(resampleOutput);
    this->OutlineSource->SetBounds(output->GetBounds());
    this->OutlineSource->GetBounds(this->DataBounds);
    this->OutlineSource->Update();

    this->DataSize = output->GetActualMemorySize();
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
