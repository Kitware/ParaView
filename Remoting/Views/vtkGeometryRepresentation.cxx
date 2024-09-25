// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGeometryRepresentation.h"
#include "vtkGeometryRepresentationInternal.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkCompositeCellGridMapper.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataObjectTypes.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLogger.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkShader.h"
#include "vtkShaderProperty.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringToken.h"
#include "vtkTexture.h"
#include "vtkTransform.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayActorNode.h"
#endif

#include <vtk_jsoncpp.h>
#include <vtksys/SystemTools.hxx>

#include <algorithm>
#include <memory>
#include <numeric>
#include <tuple>
#include <vector>

namespace vtkGeometryRepresentation_detail
{
vtkStandardNewMacro(DecimationFilterType);
}

//*****************************************************************************
// This is used to convert a vtkPolyData to a vtkPartitionedDataSetCollection.
// If the input is a vtkPartitionedDataSetCollection/vtkMultiBlockDataSet,
// then this is simply a pass-through filter. This makes it easier to unify
// the code to select and render data by simply dealing with vtkDataObjectTrees always.
class vtkGeometryRepresentationMultiBlockMaker : public vtkDataObjectAlgorithm
{
public:
  static vtkGeometryRepresentationMultiBlockMaker* New();
  vtkTypeMacro(vtkGeometryRepresentationMultiBlockMaker, vtkDataObjectAlgorithm);

protected:
  int RequestData(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
    auto inputDOT = vtkDataObjectTree::SafeDownCast(inputDO);
    auto outputDOT = vtkDataObjectTree::GetData(outputVector, 0);

    vtkInformation* infoNormals = this->GetInputArrayInformation(0);
    vtkInformation* infoTCoords = this->GetInputArrayInformation(1);
    vtkInformation* infoTangents = this->GetInputArrayInformation(2);

    const std::string normalsName = infoNormals && infoNormals->Has(vtkDataObject::FIELD_NAME())
      ? infoNormals->Get(vtkDataObject::FIELD_NAME())
      : "";
    const std::string tcoordsName = infoTCoords && infoTCoords->Has(vtkDataObject::FIELD_NAME())
      ? infoTCoords->Get(vtkDataObject::FIELD_NAME())
      : "";
    const std::string tangentsName = infoTangents && infoTangents->Has(vtkDataObject::FIELD_NAME())
      ? infoTangents->Get(vtkDataObject::FIELD_NAME())
      : "";

    if (inputDOT)
    {
      outputDOT->ShallowCopy(inputDOT);

      auto iter = vtk::TakeSmartPointer(outputDOT->NewTreeIterator());
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        this->SetArrays(iter->GetCurrentDataObject(), normalsName, tcoordsName, tangentsName);
      }
      return 1;
    }
    else
    {
      auto clone = vtkSmartPointer<vtkDataObject>::Take(inputDO->NewInstance());
      clone->ShallowCopy(inputDO);
      auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(outputDOT);
      outputPDC->SetNumberOfPartitionedDataSets(1);
      outputPDC->SetPartition(0, 0, clone);
      this->SetArrays(clone, normalsName, tcoordsName, tangentsName);

      // if we created a PDC out of a non-composite dataset, we add this array to
      // make it possible for `PopulateBlockAttributes` to be aware of that.
      vtkNew<vtkIntArray> marker;
      marker->SetName("vtkGeometryRepresentationMultiBlockMaker");
      outputPDC->GetFieldData()->AddArray(marker);
      return 1;
    }
  }

  void SetArrays(vtkDataObject* dataSet, const std::string& normal, const std::string& tcoord,
    const std::string& tangent)
  {
    if (dataSet && dataSet->GetAttributes(vtkDataObject::POINT))
    {
      auto pointData = dataSet->GetAttributes(vtkDataObject::POINT);
      if (!normal.empty() && normal != "None")
      {
        pointData->SetActiveNormals(normal.c_str());
      }
      else if (pointData->GetNormals())
      {
        pointData->SetActiveNormals(pointData->GetNormals()->GetName());
      }
      if (!tcoord.empty() && tcoord != "None")
      {
        pointData->SetActiveTCoords(tcoord.c_str());
      }
      else if (pointData->GetTCoords())
      {
        pointData->SetActiveTCoords(pointData->GetTCoords()->GetName());
      }
      if (!tangent.empty() && tangent != "None")
      {
        pointData->SetActiveTangents(tangent.c_str());
      }
      else if (pointData->GetTangents())
      {
        pointData->SetActiveTangents(pointData->GetTangents()->GetName());
      }
    }
  }

  int FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    return 1;
  }

  int RequestDataObject(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    auto inputDO = vtkDataObject::GetData(inputVector[0], 0);
    int outputType = VTK_PARTITIONED_DATA_SET_COLLECTION;
    if (inputDO && inputDO->GetDataObjectType() == VTK_MULTIBLOCK_DATA_SET)
    {
      outputType = VTK_MULTIBLOCK_DATA_SET;
    }

    return vtkDataObjectAlgorithm::SetOutputDataObject(
             outputType, outputVector->GetInformationObject(0), /*exact*/ true)
      ? 1
      : 0;
  }
};
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGeometryRepresentationMultiBlockMaker);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkGeometryRepresentation);

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::HandleGeometryRepresentationProgress(
  vtkObject* caller, unsigned long, void*)
{
  vtkAlgorithm* algorithm = vtkAlgorithm::SafeDownCast(caller);
  if (algorithm)
  {
    double progress = algorithm->GetProgress();
    if (progress > 0.0 && progress < 1.0)
    {
      if (algorithm == this->GeometryFilter)
      {
        this->UpdateProgress(progress * 0.8);
      }
      else if (algorithm == this->MultiBlockMaker)
      {
        this->UpdateProgress(0.8 + progress * 0.05);
      }
      else if (algorithm == this->Decimator)
      {
        this->UpdateProgress(0.85 + progress * 0.10);
      }
      else if (algorithm == this->LODOutlineFilter)
      {
        this->UpdateProgress(0.95 + progress * 0.05);
      }
    }
    if (this->AbortExecute)
    {
      algorithm->SetAbortExecute(1);
    }
  }
}

//----------------------------------------------------------------------------
vtkGeometryRepresentation::vtkGeometryRepresentation()
{
  this->SetActiveAssembly("Hierarchy");
  this->GeometryFilter = vtkPVGeometryFilter::New();
  this->MultiBlockMaker = vtkGeometryRepresentationMultiBlockMaker::New();
  this->Decimator = vtkGeometryRepresentation_detail::DecimationFilterType::New();
  this->LODOutlineFilter = vtkPVGeometryFilter::New();

  // connect progress bar
  this->GeometryFilter->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkGeometryRepresentation::HandleGeometryRepresentationProgress);
  this->MultiBlockMaker->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkGeometryRepresentation::HandleGeometryRepresentationProgress);
  this->Decimator->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkGeometryRepresentation::HandleGeometryRepresentationProgress);
  this->LODOutlineFilter->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkGeometryRepresentation::HandleGeometryRepresentationProgress);

  // setup the selection mapper so that we don't need to make any selection
  // conversions after rendering.
  vtkCompositePolyDataMapper* mapper = vtkCompositePolyDataMapper::New();
  mapper->SetProcessIdArrayName("vtkProcessId");
  mapper->SetCompositeIdArrayName("vtkCompositeIndex");

  this->Mapper = mapper;

  this->SetArrayIdNames(nullptr, nullptr);

  this->LODMapper = vtkCompositePolyDataMapper::New();
  this->Actor = vtkPVLODActor::New();
  this->Property = vtkProperty::New();

  this->RequestGhostCellsIfNeeded = true;
  this->RepeatTextures = true;
  this->InterpolateTextures = false;
  this->UseMipmapTextures = false;
  this->TextureTransform = nullptr;
  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->Representation = SURFACE;

  this->SuppressLOD = false;

  vtkMath::UninitializeBounds(this->VisibleDataBounds);

  this->SetupDefaults();

  this->PWF = nullptr;

  this->UseDataPartitions = false;

  this->UseShaderReplacements = false;
  this->ShaderReplacementsString = "";

  // By default, show everything.
  this->AddBlockSelector("/");
}

//----------------------------------------------------------------------------
vtkGeometryRepresentation::~vtkGeometryRepresentation()
{
  this->SetActiveAssembly(nullptr);
  this->GeometryFilter->Delete();
  this->MultiBlockMaker->Delete();
  if (this->Decimator)
  {
    this->Decimator->Delete();
  }
  this->LODOutlineFilter->Delete();
  this->Mapper->Delete();
  this->LODMapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  if (this->TextureTransform)
  {
    this->TextureTransform->Delete();
    this->TextureTransform = nullptr;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetupDefaults()
{
  // setup composite display attributes
  if (auto cpdm = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper))
  {
    vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributes;
    cpdm->SetCompositeDataDisplayAttributes(compositeAttributes);
  }

  if (auto cpdm = vtkCompositePolyDataMapper::SafeDownCast(this->LODMapper))
  {
    vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributesLOD;
    cpdm->SetCompositeDataDisplayAttributes(compositeAttributesLOD);
  }

  vtkNew<vtkSelection> sel;
  this->Mapper->SetSelection(sel);

  this->Decimator->SetLODFactor(0.5);

  this->LODOutlineFilter->SetUseOutline(1);

  vtkPVGeometryFilter* geomFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter);
  if (geomFilter)
  {
    geomFilter->SetUseOutline(0);
    geomFilter->SetTriangulate(0);
    geomFilter->SetNonlinearSubdivisionLevel(1);
    geomFilter->SetMatchBoundariesIgnoringCellOrder(0);
    geomFilter->SetPassThroughCellIds(1);
    geomFilter->SetPassThroughPointIds(1);
  }

  this->MultiBlockMaker->SetInputConnection(this->GeometryFilter->GetOutputPort());

  this->Actor->SetMapper(this->Mapper);
  this->Actor->SetLODMapper(this->LODMapper);
  this->Actor->SetProperty(this->Property);

  // Not insanely thrilled about this API on vtkProp about properties, but oh
  // well. We have to live with it.
  vtkInformation* keys = vtkInformation::New();
  this->Actor->SetPropertyKeys(keys);
  keys->Delete();
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::GetBlockColorsDistinctValues()
{
  vtkPVGeometryFilter* geomFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter);
  if (geomFilter)
  {
    return geomFilter->GetBlockColorsDistinctValues();
  }
  return 2;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockColorsDistinctValues(int distinctValues)
{
  vtkPVGeometryFilter* geomFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter);
  if (geomFilter)
  {
    geomFilter->SetBlockColorsDistinctValues(distinctValues);
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    // i.e. this->GetVisibility() == false, hence nothing to do.
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // provide the "geometry" to the view so the view can deliver it to the
    // rendering nodes as and when needed.
    vtkPVView::SetPiece(inInfo, this, this->MultiBlockMaker->GetOutputDataObject(0));

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

    outInfo->Set(
      vtkPVRenderView::NEED_ORDERED_COMPOSITING(), this->NeedsOrderedCompositing() ? 1 : 0);

    // Finally, let the view know about the geometry bounds. The view uses this
    // information for resetting camera and clip planes. Since this
    // representation allows users to transform the geometry, we need to ensure
    // that the bounds we report include the transformation as well.
    this->ComputeVisibleDataBounds();

    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix);
    vtkPVRenderView::SetGeometryBounds(inInfo, this, this->VisibleDataBounds, matrix);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    // Called to generate and provide the LOD data to the view.
    // If SuppressLOD is true, we tell the view we have no LOD data to provide,
    // otherwise we provide the decimated data.
    auto data = vtkPVView::GetPiece(inInfo, this);
    if (data != nullptr && !this->SuppressLOD)
    {
      if (inInfo->Has(vtkPVRenderView::USE_OUTLINE_FOR_LOD()))
      {
        this->LODOutlineFilter->SetInputDataObject(data);
        this->LODOutlineFilter->Update();
        // Pass along the LOD geometry to the view so that it can deliver it to
        // the rendering node as and when needed.
        vtkPVView::SetPieceLOD(inInfo, this, this->LODOutlineFilter->GetOutputDataObject(0));
      }
      else
      {
        if (inInfo->Has(vtkPVRenderView::LOD_RESOLUTION()))
        {
          // We handle this number differently depending on decimator
          // implementation.
          const double factor = inInfo->Get(vtkPVRenderView::LOD_RESOLUTION());
          this->Decimator->SetLODFactor(factor);
        }

        this->Decimator->SetInputDataObject(data);
        this->Decimator->Update();

        // Pass along the LOD geometry to the view so that it can deliver it to
        // the rendering node as and when needed.
        vtkPVView::SetPieceLOD(inInfo, this, this->Decimator->GetOutputDataObject(0));
      }
    }
  }
  else if (request_type == vtkPVView::REQUEST_RENDER())
  {
    auto outputData = vtkPVView::GetDeliveredPiece(inInfo, this);
    // vtkLogF(INFO, "%p: %s", (void*)data, this->GetLogName().c_str());
    auto dataLOD = vtkPVView::GetDeliveredPieceLOD(inInfo, this);
    this->Mapper->SetInputDataObject(outputData);
    this->LODMapper->SetInputDataObject(dataLOD);

    // This is called just before the vtk-level render. In this pass, we simply
    // pick the correct rendering mode and rendering parameters.
    bool lod = this->SuppressLOD ? false : (inInfo->Has(vtkPVRenderView::USE_LOD()) == 1);
    this->Actor->SetEnableLOD(lod ? 1 : 0);
    this->UpdateColoringParameters();

    if (outputData && (this->BlockAttributeTime < outputData->GetMTime() || this->BlockAttrChanged))
    {
      if (auto cmapper = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper))
      {
        this->PopulateBlockAttributes(cmapper->GetCompositeDataDisplayAttributes(), outputData);
      }
      this->BlockAttributeTime.Modified();
      this->BlockAttrChanged = false;

      // This flag makes the following LOD render requests to also update the block
      // attribute state, if there were any changes.
      this->UpdateBlockAttrLOD = true;
    }

    if (lod && dataLOD && this->UpdateBlockAttrLOD)
    {
      if (auto cmapper = vtkCompositePolyDataMapper::SafeDownCast(this->LODMapper))
      {
        this->PopulateBlockAttributes(cmapper->GetCompositeDataDisplayAttributes(), dataLOD);
      }
      this->UpdateBlockAttrLOD = false;
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  // ensure that the ghost-level information is setup correctly to avoid
  // internal faces for unstructured grids.
  for (int cc = 0; cc < this->GetNumberOfInputPorts(); cc++)
  {
    for (int kk = 0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
    {
      vtkInformation* inInfo = inputVector[cc]->GetInformationObject(kk);
      int ghostLevels =
        inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
      if (this->RequestGhostCellsIfNeeded)
      {
        ghostLevels += vtkProcessModule::GetNumberOfGhostLevelsToRequest(inInfo);
      }
      inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (inputVector[0]->GetNumberOfInformationObjects() == 1)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkAlgorithmOutput* internalOutputPort = this->GetInternalOutputPort();
    auto prod = vtkPVTrivialProducer::SafeDownCast(internalOutputPort->GetProducer());
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) && prod)
    {
      prod->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
    }
    this->GeometryFilter->SetInputConnection(internalOutputPort);
  }
  else
  {
    auto placeHolder =
      vtk::TakeSmartPointer(vtkDataObjectTypes::NewDataObject(this->PlaceHolderDataType));
    placeHolder->Initialize();
    this->GeometryFilter->SetInputDataObject(0, placeHolder);
  }

  // essential to re-execute geometry filter consistently on all ranks since it
  // does use parallel communication (see #19963).
  // do this only when multiple processes exists, so GeometryFilter can do
  // some "filter unmodified" optimization.
  auto controller = vtkMultiProcessController::GetGlobalController();
  if (controller->GetNumberOfProcesses() > 1)
  {
    this->GeometryFilter->Modified();
  }
  this->MultiBlockMaker->Update();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::GetBounds(
  vtkDataObject* dataObject, double bounds[6], vtkCompositeDataDisplayAttributes* cdAttributes)
{
  vtkMath::UninitializeBounds(bounds);
  if (vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dataObject))
  {
    // computing bounds with only visible blocks
    vtkCompositeDataDisplayAttributes::ComputeVisibleBounds(cdAttributes, cd, bounds);
    if (vtkBoundingBox::IsValid(bounds))
    {
      return true;
    }
  }
  else if (vtkDataSet* ds = vtkDataSet::SafeDownCast(dataObject))
  {
    ds->GetBounds(bounds);
    return (vtkMath::AreBoundsInitialized(bounds) == 1);
  }
  else if (vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(dataObject))
  {
    htg->GetBounds(bounds);
    return (vtkMath::AreBoundsInitialized(bounds) == 1);
  }
  return false;
}

//----------------------------------------------------------------------------
vtkDataObject* vtkGeometryRepresentation::GetRenderedDataObject(int vtkNotUsed(port))
{
  if (this->GeometryFilter->GetNumberOfInputConnections(0) > 0)
  {
    return this->MultiBlockMaker->GetOutputDataObject(0);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->Actor);

    // Indicate that this is prop that we are rendering when hardware selection
    // is enabled.
    rview->RegisterPropForHardwareSelection(this, this->GetRenderedProp());
    return this->Superclass::AddToView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->Actor);
    rview->UnRegisterPropForHardwareSelection(this, this->GetRenderedProp());
    return this->Superclass::RemoveFromView(view);
  }
  return false;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetRepresentation(const char* type)
{
  if (vtksys::SystemTools::Strucmp(type, "Points") == 0)
  {
    this->SetRepresentation(POINTS);
  }
  else if (vtksys::SystemTools::Strucmp(type, "Wireframe") == 0)
  {
    this->SetRepresentation(WIREFRAME);
  }
  else if (vtksys::SystemTools::Strucmp(type, "Surface") == 0)
  {
    this->SetRepresentation(SURFACE);
  }
  else if (vtksys::SystemTools::Strucmp(type, "Surface With Edges") == 0)
  {
    this->SetRepresentation(SURFACE_WITH_EDGES);
  }
  else
  {
    vtkErrorMacro("Invalid type: " << type);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetPointArrayToProcess(int p, const char* val)
{
  this->MultiBlockMaker->SetInputArrayToProcess(
    p, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, val);
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetNormalArray(const char* val)
{
  this->SetPointArrayToProcess(0, val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetTCoordArray(const char* val)
{
  this->SetPointArrayToProcess(1, val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetTangentArray(const char* val)
{
  this->SetPointArrayToProcess(2, val);
}

//----------------------------------------------------------------------------
const char* vtkGeometryRepresentation::GetColorArrayName()
{
  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    return info->Get(vtkDataObject::FIELD_NAME());
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::UpdateColoringParameters()
{
  bool using_scalar_coloring = false;

  vtkInformation* info = this->GetInputArrayInformation(0);
  if (info && info->Has(vtkDataObject::FIELD_ASSOCIATION()) &&
    info->Has(vtkDataObject::FIELD_NAME()))
  {
    const char* colorArrayName = info->Get(vtkDataObject::FIELD_NAME());
    int fieldAssociation = info->Get(vtkDataObject::FIELD_ASSOCIATION());
    if (colorArrayName && colorArrayName[0])
    {
      this->Mapper->SetScalarVisibility(1);
      this->LODMapper->SetScalarVisibility(1);
      this->Mapper->SelectColorArray(colorArrayName);
      this->LODMapper->SelectColorArray(colorArrayName);
      this->Mapper->SetUseLookupTableScalarRange(1);
      this->LODMapper->SetUseLookupTableScalarRange(1);
      switch (fieldAssociation)
      {
        case vtkDataObject::FIELD_ASSOCIATION_CELLS:
          this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
          this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
          break;

        case vtkDataObject::FIELD_ASSOCIATION_NONE:
          this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
          // Color entire block by zeroth tuple in the field data
          this->Mapper->SetFieldDataTupleId(0);
          this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_FIELD_DATA);
          this->LODMapper->SetFieldDataTupleId(0);
          break;

        case vtkDataObject::FIELD_ASSOCIATION_POINTS:
        default:
          this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
          this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
          break;
      }
      using_scalar_coloring = true;
    }
  }

  if (!using_scalar_coloring)
  {
    this->Mapper->SetScalarVisibility(0);
    this->LODMapper->SetScalarVisibility(0);
    this->Mapper->SelectColorArray(nullptr);
    this->LODMapper->SelectColorArray(nullptr);
  }

  // Adjust material properties.
  double diffuse = this->Diffuse;
  double specular = this->Specular;
  double ambient = this->Ambient;
  bool lighting = !this->DisableLighting;

  if (this->Representation != SURFACE && this->Representation != SURFACE_WITH_EDGES)
  {
    if ((this->Representation == WIREFRAME && this->Property->GetRenderLinesAsTubes()) ||
      (this->Representation == POINTS && this->Property->GetRenderPointsAsSpheres()))
    {
      // use diffuse lighting, since we're rendering as tubes or spheres.
    }
    else
    {
      diffuse = 0.0;
      ambient = 1.0;
    }
  }

  this->Property->SetAmbient(ambient);
  this->Property->SetSpecular(specular);
  this->Property->SetDiffuse(diffuse);
  this->Property->SetLighting(lighting);

  switch (this->Representation)
  {
    case SURFACE_WITH_EDGES:
      this->Property->SetEdgeVisibility(1);
      this->Property->SetRepresentation(VTK_SURFACE);
      break;

    default:
      this->Property->SetEdgeVisibility(0);
      this->Property->SetRepresentation(this->Representation);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoordinateShiftScaleMethod(int val)
{
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(this->Mapper);
  if (mapper)
  {
    mapper->SetVBOShiftScaleMethod(val);
  }
}

int vtkGeometryRepresentation::GetCoordinateShiftScaleMethod()
{
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(this->Mapper);
  if (mapper)
  {
    return mapper->GetVBOShiftScaleMethod();
  }
  return 0;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::NeedsOrderedCompositing()
{
  // One would think simply calling `vtkActor::HasTranslucentPolygonalGeometry`
  // should do the trick, however that method relies on the mapper's input
  // having up-to-date data. vtkGeometryRepresentation needs to determine
  // whether the representation needs ordered compositing in `REQUEST_UPDATE`
  // pass i.e. before the mapper's input is updated. Hence we explicitly
  // determine if the mapper may choose to render translucent geometry.
  if (this->Actor->GetForceOpaque())
  {
    return false;
  }

  if (this->Actor->GetForceTranslucent())
  {
    return true;
  }

  if (auto prop = this->Actor->GetProperty())
  {
    auto opacity = prop->GetOpacity();
    if (opacity > 0.0 && opacity < 1.0)
    {
      return true;
    }
  }

  if (auto texture = this->Actor->GetTexture())
  {
    if (texture->IsTranslucent())
    {
      return true;
    }
  }

  // Check is BlockOpacities has any value not 0 or 1.
  if (std::accumulate(this->BlockOpacities.begin(), this->BlockOpacities.end(), false,
        [](bool result, const std::pair<std::string, double>& apair) {
          return result || (apair.second > 0.0 && apair.second < 1.0);
        }))
  {
    // a translucent block may be present.
    return true;
  }

  auto colorarrayname = this->GetColorArrayName();
  if (colorarrayname && colorarrayname[0])
  {
    if (this->Mapper->GetColorMode() == VTK_COLOR_MODE_DIRECT_SCALARS)
    {
      // when mapping scalars directly, assume the scalars have an alpha
      // component since we cannot check if that is indeed the case consistently
      // on all ranks without a bit of work.
      return true;
    }

    if (auto lut = this->Mapper->GetLookupTable())
    {
      if (lut->IsOpaque() == 0)
      {
        return true;
      }
    }
  }

  return false;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//****************************************************************************
// Methods merely forwarding parameters to internal objects.
//****************************************************************************

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->Mapper->SetLookupTable(val);
  this->LODMapper->SetLookupTable(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetColorMissingArraysWithNanColor(bool val)
{
  if (auto cpdm = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper))
  {
    cpdm->SetColorMissingArraysWithNanColor(val);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetMapScalars(int val)
{
  if (val < 0 || val > 1)
  {
    vtkWarningMacro(<< "Invalid parameter for vtkGeometryRepresentation::SetMapScalars: " << val);
    val = 0;
  }
  int mapToColorMode[] = { VTK_COLOR_MODE_DIRECT_SCALARS, VTK_COLOR_MODE_MAP_SCALARS };
  this->Mapper->SetColorMode(mapToColorMode[val]);
  this->LODMapper->SetColorMode(mapToColorMode[val]);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetInterpolateScalarsBeforeMapping(int val)
{
  this->Mapper->SetInterpolateScalarsBeforeMapping(val);
  this->LODMapper->SetInterpolateScalarsBeforeMapping(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetStatic(int val)
{
  this->Mapper->SetStatic(val);
  this->LODMapper->SetStatic(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetColor(double r, double g, double b)
{
  this->Property->SetColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetLighting(bool lighting)
{
  this->Property->SetLighting(lighting);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetLineWidth(double val)
{
  this->Property->SetLineWidth(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetOpacity(double val)
{
  this->Property->SetOpacity(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetEdgeOpacity(double val)
{
  this->Property->SetEdgeOpacity(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetLuminosity(double val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  vtkOSPRayActorNode::SetLuminosity(val, this->Property);
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetRenderPointsAsSpheres(bool val)
{
  this->Property->SetRenderPointsAsSpheres(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetRenderLinesAsTubes(bool val)
{
  this->Property->SetRenderLinesAsTubes(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetRoughness(double val)
{
  this->Property->SetRoughness(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetMetallic(double val)
{
  this->Property->SetMetallic(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBaseIOR(double val)
{
  this->Property->SetBaseIOR(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoatIOR(double val)
{
  this->Property->SetCoatIOR(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoatStrength(double val)
{
  this->Property->SetCoatStrength(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoatRoughness(double val)
{
  this->Property->SetCoatRoughness(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoatNormalScale(double val)
{
  this->Property->SetCoatNormalScale(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoatColor(double r, double g, double b)
{
  this->Property->SetCoatColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetEdgeTint(double r, double g, double b)
{
  this->Property->SetEdgeTint(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetAnisotropy(double val)
{
  this->Property->SetAnisotropy(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetAnisotropyRotation(double val)
{
  this->Property->SetAnisotropyRotation(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBaseColorTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOn();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetBaseColorTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetMaterialTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOff();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetORMTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetAnisotropyTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOff();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetAnisotropyTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetNormalTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOff();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetNormalTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetCoatNormalTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOff();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetCoatNormalTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetEmissiveTexture(vtkTexture* tex)
{
  if (tex)
  {
    tex->UseSRGBColorSpaceOn();
    tex->SetInterpolate(this->InterpolateTextures);
    tex->SetRepeat(this->RepeatTextures);
    tex->SetMipmap(this->UseMipmapTextures);
  }
  this->Property->SetEmissiveTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetNormalScale(double val)
{
  this->Property->SetNormalScale(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetOcclusionStrength(double val)
{
  this->Property->SetOcclusionStrength(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetEmissiveFactor(double rval, double gval, double bval)
{
  this->Property->SetEmissiveFactor(rval, gval, bval);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetPointSize(double val)
{
  this->Property->SetPointSize(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetAmbientColor(double r, double g, double b)
{
  this->Property->SetAmbientColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetDiffuseColor(double r, double g, double b)
{
  this->Property->SetDiffuseColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetEdgeColor(double r, double g, double b)
{
  this->Property->SetEdgeColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetInteractiveSelectionColor(double r, double g, double b)
{
  this->Property->SetSelectionColor(r, g, b, 1.0);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetInterpolation(int val)
{
  this->Property->SetInterpolation(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetSpecularColor(double r, double g, double b)
{
  this->Property->SetSpecularColor(r, g, b);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetSpecularPower(double val)
{
  this->Property->SetSpecularPower(val);
}

void vtkGeometryRepresentation::SetShowTexturesOnBackface(bool val)
{
  this->Property->SetShowTexturesOnBackface(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetOrientation(double x, double y, double z)
{
  this->Actor->SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetOrigin(double x, double y, double z)
{
  this->Actor->SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetPickable(int val)
{
  this->Actor->SetPickable(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetPosition(double x, double y, double z)
{
  this->Actor->SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetScale(double x, double y, double z)
{
  this->Actor->SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetUserTransform(const double matrix[16])
{
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix);
  this->Actor->SetUserTransform(transform);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetSelection(vtkSelection* selection)
{
  // we need to shallow copy the existing selection instead of changing it in order to avoid
  // changing the MTime of the mapper to avoid rebuilding everything
  this->Mapper->GetSelection()->ShallowCopy(selection);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetFlipTextures(bool flip)
{
  if (this->TextureTransform)
  {
    if (flip)
    {
      static constexpr double mat[] = { 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
      this->TextureTransform->SetMatrix(mat);
      this->TextureTransform->Modified();
    }
    else
    {
      static constexpr double mat[] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
      this->TextureTransform->SetMatrix(mat);
      this->TextureTransform->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetTexture(vtkTexture* val)
{
  this->Actor->SetTexture(val);
  if (val)
  {
    val->SetRepeat(this->RepeatTextures);
    val->SetInterpolate(this->InterpolateTextures);
    val->SetMipmap(this->UseMipmapTextures);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetTextureTransform(vtkTransform* transform)
{
  vtkSetObjectBodyMacro(TextureTransform, vtkTransform, transform);
  if (this->TextureTransform && this->Actor &&
    !this->TextureTransform->HasObserver(vtkCommand::ModifiedEvent))
  {
    this->UpdateGeneralTextureTransform();
    this->TextureTransform->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkGeometryRepresentation::UpdateGeneralTextureTransform);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::UpdateGeneralTextureTransform()
{
  if (this->Actor && this->Actor->GetPropertyKeys())
  {
    vtkInformation* info = this->Actor->GetPropertyKeys();
    info->Remove(vtkProp::GeneralTextureTransform());
    info->Set(vtkProp::GeneralTextureTransform(),
      &(this->TextureTransform->GetMatrix()->Element[0][0]), 16);
    this->Actor->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetRepeatTextures(bool rep)
{
  if (this->Actor->GetTexture())
  {
    this->Actor->GetTexture()->SetRepeat(rep);
  }
  std::map<std::string, vtkTexture*>& tex = this->Actor->GetProperty()->GetAllTextures();
  for (auto t : tex)
  {
    t.second->SetRepeat(rep);
  }
  this->RepeatTextures = rep;
}

void vtkGeometryRepresentation::SetInterpolateTextures(bool rep)
{
  if (this->Actor->GetTexture())
  {
    this->Actor->GetTexture()->SetInterpolate(rep);
  }
  std::map<std::string, vtkTexture*>& tex = this->Actor->GetProperty()->GetAllTextures();
  for (auto t : tex)
  {
    t.second->SetInterpolate(rep);
  }
  this->InterpolateTextures = rep;
}

void vtkGeometryRepresentation::SetUseMipmapTextures(bool rep)
{
  if (this->Actor->GetTexture())
  {
    this->Actor->GetTexture()->SetMipmap(rep);
  }
  std::map<std::string, vtkTexture*>& tex = this->Actor->GetProperty()->GetAllTextures();
  for (auto t : tex)
  {
    t.second->SetMipmap(rep);
  }
  this->UseMipmapTextures = rep;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetSeamlessU(bool rep)
{
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(this->Mapper);
  if (mapper)
  {
    mapper->SetSeamlessU(rep);
  }
  vtkPolyDataMapper* lod = vtkPolyDataMapper::SafeDownCast(this->LODMapper);
  if (lod)
  {
    lod->SetSeamlessU(rep);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetSeamlessV(bool rep)
{
  vtkPolyDataMapper* mapper = vtkPolyDataMapper::SafeDownCast(this->Mapper);
  if (mapper)
  {
    mapper->SetSeamlessV(rep);
  }
  vtkPolyDataMapper* lod = vtkPolyDataMapper::SafeDownCast(this->LODMapper);
  if (lod)
  {
    lod->SetSeamlessV(rep);
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetUseOutline(int val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetUseOutline(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetTriangulate(int val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetTriangulate(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetNonlinearSubdivisionLevel(int val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetNonlinearSubdivisionLevel(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetMatchBoundariesIgnoringCellOrder(int val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetMatchBoundariesIgnoringCellOrder(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetGenerateFeatureEdges(bool val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetGenerateFeatureEdges(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetComputePointNormals(bool val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetGeneratePointNormals(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetSplitting(bool val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetSplitting(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetFeatureAngle(double val)
{
  if (auto geometryFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    geometryFilter->SetFeatureAngle(val);
  }
  // since geometry filter needs to execute, we need to mark the representation modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetPlaceHolderDataType(int datatype)
{
  if (this->PlaceHolderDataType != datatype)
  {
    this->PlaceHolderDataType = datatype;
    this->MarkModified();
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::AddBlockSelector(const char* selector)
{
  if (selector != nullptr &&
    std::find(this->BlockSelectors.begin(), this->BlockSelectors.end(), selector) ==
      this->BlockSelectors.end())
  {
    this->BlockSelectors.push_back(selector);
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockSelectors()
{
  if (!this->BlockSelectors.empty())
  {
    this->BlockSelectors.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockColor(const char* selector, double r, double g, double b)
{
  if (selector != nullptr)
  {
    auto iter = std::find_if(this->BlockColors.begin(), this->BlockColors.end(),
      [selector](
        const std::pair<std::string, vtkVector3d>& apair) { return apair.first == selector; });
    if (iter == this->BlockColors.end())
    {
      this->BlockColors.emplace_back(selector, vtkVector3d(r, g, b));
      this->BlockAttrChanged = true;
    }
    else if (iter->second != vtkVector3d(r, g, b))
    {
      iter->second = vtkVector3d(r, g, b);
      this->BlockAttrChanged = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockColors()
{
  if (!this->BlockColors.empty())
  {
    this->BlockColors.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockOpacity(const char* selector, double alpha)
{
  if (selector != nullptr)
  {
    auto iter = std::find_if(this->BlockOpacities.begin(), this->BlockOpacities.end(),
      [selector](const std::pair<std::string, double>& apair) { return apair.first == selector; });
    if (iter == this->BlockOpacities.end())
    {
      this->BlockOpacities.emplace_back(selector, alpha);
      this->BlockAttrChanged = true;
    }
    else if (iter->second != alpha)
    {
      iter->second = alpha;
      this->BlockAttrChanged = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockOpacities()
{
  if (!this->BlockOpacities.empty())
  {
    this->BlockOpacities.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockInterpolateScalarsBeforeMapping(
  const char* selector, bool interpolate)
{
  if (selector != nullptr)
  {
    auto iter = std::find_if(this->BlockInterpolateScalarsBeforeMapping.begin(),
      this->BlockInterpolateScalarsBeforeMapping.end(),
      [selector](const std::pair<std::string, bool>& apair) { return apair.first == selector; });
    if (iter == this->BlockInterpolateScalarsBeforeMapping.end())
    {
      this->BlockInterpolateScalarsBeforeMapping.emplace_back(selector, interpolate);
      this->BlockAttrChanged = true;
    }
    else if (iter->second != interpolate)
    {
      iter->second = interpolate;
      this->BlockAttrChanged = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockInterpolateScalarsBeforeMappings()
{
  if (!this->BlockInterpolateScalarsBeforeMapping.empty())
  {
    this->BlockInterpolateScalarsBeforeMapping.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockMapScalars(const char* selector, int val)
{
  if (val < 0 || val > 1)
  {
    vtkWarningMacro(<< "Invalid parameter for vtkGeometryRepresentation::SetBlockMapScalars: "
                    << val);
    val = 0;
  }
  static const int mapToColorMode[] = { VTK_COLOR_MODE_DIRECT_SCALARS, VTK_COLOR_MODE_MAP_SCALARS };
  const int colorMode = mapToColorMode[val];
  if (selector != nullptr)
  {
    auto iter = std::find_if(this->BlockColorModes.begin(), this->BlockColorModes.end(),
      [selector](const std::pair<std::string, int>& apair) { return apair.first == selector; });
    if (iter == this->BlockColorModes.end())
    {
      this->BlockColorModes.emplace_back(selector, colorMode);
      this->BlockAttrChanged = true;
    }
    else if (iter->second != colorMode)
    {
      iter->second = colorMode;
      this->BlockAttrChanged = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockMapScalars()
{
  if (!this->BlockColorModes.empty())
  {
    this->BlockColorModes.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockArrayName(
  const char* selector, int assoc, const char* colorArray)
{
  if (selector != nullptr && colorArray != nullptr)
  {
    auto iter = std::find_if(this->BlockArrayNames.begin(), this->BlockArrayNames.end(),
      [selector](const std::pair<std::string, std::pair<int, std::string>>& apair) {
        return apair.first == selector;
      });
    if (iter == this->BlockArrayNames.end())
    {
      this->BlockArrayNames.emplace_back(selector, std::make_pair(assoc, colorArray));
      this->BlockAttrChanged = true;
    }
    else if (iter->second.first != assoc && iter->second.second != colorArray)
    {
      iter->second = std::make_pair(assoc, colorArray);
      this->BlockAttrChanged = true;
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockArrayNames()
{
  if (!this->BlockArrayNames.empty())
  {
    this->BlockArrayNames.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockLookupTable(vtkScalarsToColors* lut)
{
  if (lut != nullptr)
  {
    this->BlockLookupTables.push_back(lut);
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveAllBlockLookupTables()
{
  if (!this->BlockLookupTables.empty())
  {
    this->BlockLookupTables.clear();
    this->BlockAttrChanged = true;
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::PopulateBlockAttributes(
  vtkCompositeDataDisplayAttributes* attrs, vtkDataObject* outputData)
{
  // exposed properties
  attrs->RemoveBlockVisibilities();
  attrs->RemoveBlockOpacities();
  attrs->RemoveBlockColors();
  attrs->RemoveBlockInterpolateScalarsBeforeMappings();
  attrs->RemoveBlockColorModes();
  attrs->RemoveBlockScalarModes();
  attrs->RemoveBlockArrayNames();
  attrs->RemoveBlockLookupTables();
  // internal properties
  attrs->RemoveBlockScalarVisibilities();
  attrs->RemoveBlockUseLookupTableScalarRanges();
  attrs->RemoveBlockFieldDataTupleIds();

  auto dtree = vtkDataObjectTree::SafeDownCast(outputData);
  if (dtree == nullptr ||
    dtree->GetFieldData()->GetArray("vtkGeometryRepresentationMultiBlockMaker") != nullptr)
  {
    if (dtree)
    {
      attrs->SetBlockVisibility(dtree, true); // make the root visible.
    }
    return;
  }

  std::unordered_map<unsigned int, vtkDataObject*> cid_to_dobj;
  cid_to_dobj[0u] = dtree;
  for (const auto& ref : vtk::Range(dtree,
         vtk::DataObjectTreeOptions::TraverseSubTree | vtk::DataObjectTreeOptions::SkipEmptyNodes))
  {
    cid_to_dobj[ref.GetFlatIndex()] = ref.GetDataObject();
  }

  // Handle visibilities.
  attrs->SetBlockVisibility(dtree, false); // start by marking root invisible first.
  // get the selectors for block properties
  std::set<std::string> blockPropertiesSelectorsSet;
  for (const auto& item : this->BlockColors)
  {
    blockPropertiesSelectorsSet.emplace(item.first);
  }
  for (const auto& item : this->BlockOpacities)
  {
    blockPropertiesSelectorsSet.emplace(item.first);
  }
  for (const auto& item : this->BlockInterpolateScalarsBeforeMapping)
  {
    blockPropertiesSelectorsSet.emplace(item.first);
  }
  for (const auto& item : this->BlockColorModes)
  {
    blockPropertiesSelectorsSet.emplace(item.first);
  }
  for (const auto& item : this->BlockArrayNames)
  {
    blockPropertiesSelectorsSet.emplace(item.first);
  }

  // create a vector of selectors for block properties
  const std::vector<std::string> blockPropertiesSelectors(
    blockPropertiesSelectorsSet.begin(), blockPropertiesSelectorsSet.end());
  std::vector<unsigned int> cids;
  std::unordered_map<std::string, std::vector<unsigned int>> selectorsCids;

  const auto outputPDC = vtkPartitionedDataSetCollection::SafeDownCast(outputData);
  const bool hasAssembly = outputPDC && outputPDC->GetDataAssembly();
  const bool isAssemblySelected =
    this->ActiveAssembly != nullptr && strcmp(this->ActiveAssembly, "Assembly") == 0;
  if (hasAssembly && isAssemblySelected)
  {
    // we need to convert assembly selectors to composite ids.
    cids = vtkDataAssemblyUtilities::GetSelectedCompositeIds(
      this->BlockSelectors, outputPDC->GetDataAssembly(), outputPDC);

    // create a composite ids map for the assembly selectors.
    for (const auto& selector : blockPropertiesSelectors)
    {
      const auto selectorCIds = vtkDataAssemblyUtilities::GetSelectedCompositeIds(
        { selector }, outputPDC->GetDataAssembly(), outputPDC);
      selectorsCids[selector] = selectorCIds;
    }
  }
  else
  {
    const vtkNew<vtkDataAssembly> hierarchy;
    if (!vtkDataAssemblyUtilities::GenerateHierarchy(dtree, hierarchy))
    {
      return;
    }
    // compute the composite ids for the hierarchy selectors
    cids = vtkDataAssemblyUtilities::GetSelectedCompositeIds(this->BlockSelectors, hierarchy);
    // create a composite ids map for the hierarchy selectors.
    for (const auto& selector : blockPropertiesSelectors)
    {
      const auto selectorCIds =
        vtkDataAssemblyUtilities::GetSelectedCompositeIds({ selector }, hierarchy);
      selectorsCids[selector] = selectorCIds;
    }
  }
  // Handle visibility.
  for (const auto& id : cids)
  {
    auto iter = cid_to_dobj.find(id);
    if (iter != cid_to_dobj.end())
    {
      attrs->SetBlockVisibility(iter->second, true);
    }
  }

  // Handle color.
  for (const auto& item : this->BlockColors)
  {
    const auto& ids = selectorsCids[item.first];
    for (const auto& id : ids)
    {
      auto iter = cid_to_dobj.find(id);
      if (iter != cid_to_dobj.end())
      {
        attrs->SetBlockColor(iter->second, item.second.GetData());
        attrs->SetBlockScalarVisibility(iter->second, false);
      }
    }
  }

  // Handle opacity.
  for (const auto& item : this->BlockOpacities)
  {
    const auto& ids = selectorsCids[item.first];
    for (const auto& id : ids)
    {
      auto iter = cid_to_dobj.find(id);
      if (iter != cid_to_dobj.end())
      {
        attrs->SetBlockOpacity(iter->second, item.second);
      }
    }
  }

  // Handle InterpolateScalarsBeforeMapping.
  for (const auto& item : this->BlockInterpolateScalarsBeforeMapping)
  {
    const auto& ids = selectorsCids[item.first];
    for (const auto& id : ids)
    {
      auto iter = cid_to_dobj.find(id);
      if (iter != cid_to_dobj.end())
      {
        attrs->SetBlockInterpolateScalarsBeforeMapping(iter->second, item.second);
      }
    }
  }

  // Handle color mode.
  for (const auto& item : this->BlockColorModes)
  {
    const auto& ids = selectorsCids[item.first];
    for (const auto& id : ids)
    {
      auto iter = cid_to_dobj.find(id);
      if (iter != cid_to_dobj.end())
      {
        attrs->SetBlockColorMode(iter->second, item.second);
      }
    }
  }

  // Handle array names.
  for (const auto& item : this->BlockArrayNames)
  {
    const auto& ids = selectorsCids[item.first];
    for (const auto& id : ids)
    {
      auto iter = cid_to_dobj.find(id);
      if (iter != cid_to_dobj.end())
      {
        switch (/*assoc*/ item.second.first)
        {
          case vtkDataObject::FIELD_ASSOCIATION_CELLS:
            attrs->SetBlockScalarMode(iter->second, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
            break;

          case vtkDataObject::FIELD_ASSOCIATION_NONE:
            attrs->SetBlockScalarMode(iter->second, VTK_SCALAR_MODE_USE_FIELD_DATA);
            // Color entire block by zeroth tuple in the field data
            attrs->SetBlockFieldDataTupleId(iter->second, 0);
            break;

          case vtkDataObject::FIELD_ASSOCIATION_POINTS:
          default:
            attrs->SetBlockScalarMode(iter->second, VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
            break;
        }
        attrs->SetBlockArrayName(iter->second, item.second.second);
        attrs->SetBlockScalarVisibility(iter->second, true);
        attrs->SetBlockUseLookupTableScalarRange(iter->second, true);
      }
    }
  }

  // Handle lookup tables
  if (this->BlockLookupTables.size() == this->BlockArrayNames.size())
  {
    for (size_t i = 0, numLUTs = this->BlockLookupTables.size(); i < numLUTs; ++i)
    {
      const auto& ids = selectorsCids[this->BlockArrayNames[i].first];
      for (const auto& id : ids)
      {
        auto iter = cid_to_dobj.find(id);
        if (iter != cid_to_dobj.end())
        {
          attrs->SetBlockLookupTable(iter->second, this->BlockLookupTables[i]);
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetEnableScaling(int val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Actor->SetEnableScaling(val);
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetScalingArrayName(const char* val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Actor->SetScalingArrayName(val);
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetScalingFunction(vtkPiecewiseFunction* pwf)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  this->Actor->SetScalingFunction(pwf);
#else
  (void)pwf;
#endif
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetMaterial(const char* val)
{
#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
  if (!strcmp(val, "None"))
  {
    this->Property->SetMaterialName(nullptr);
  }
  else
  {
    this->Property->SetMaterialName(val);
  }
#else
  (void)val;
#endif
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::ComputeVisibleDataBounds()
{
  if (this->VisibleDataBoundsTime < this->GetPipelineDataTime() ||
    (this->BlockAttrChanged && this->VisibleDataBoundsTime < this->BlockAttributeTime))
  {
    // If the input data is a composite dataset, use the currently set values for block
    // visibility rather than the cached ones from the last render.  This must be computed
    // in the REQUEST_UPDATE pass but the data is only copied to the mapper in the
    // REQUEST_RENDER pass.  This constructs a dummy vtkCompositeDataDisplayAttributes
    // with only the visibilities set and calls the helper function to compute the visible
    // bounds with that.
    vtkDataObject* outputData = this->MultiBlockMaker->GetOutputDataObject(0);
    vtkNew<vtkCompositeDataDisplayAttributes> cdAttributes;
    this->PopulateBlockAttributes(cdAttributes, outputData);
    this->GetBounds(outputData, this->VisibleDataBounds, cdAttributes);
    this->VisibleDataBoundsTime.Modified();
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetUseShaderReplacements(bool useShaderRepl)
{
  if (this->UseShaderReplacements != useShaderRepl)
  {
    this->UseShaderReplacements = useShaderRepl;
    this->Modified();
    this->UpdateShaderReplacements();
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetShaderReplacements(const char* replacementsString)
{
  if (replacementsString != this->ShaderReplacementsString)
  {
    this->ShaderReplacementsString = std::string(replacementsString);
    this->Modified();
    this->UpdateShaderReplacements();
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::UpdateShaderReplacements()
{
  vtkShaderProperty* props = this->Actor->GetShaderProperty();

  if (!props)
  {
    return;
  }

  props->ClearAllShaderReplacements();

  if (!this->UseShaderReplacements || this->ShaderReplacementsString.empty())
  {
    return;
  }

  Json::CharReaderBuilder builder;
  builder["collectComments"] = false;
  Json::Value root;
  std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
  bool success = reader->parse(this->ShaderReplacementsString.c_str(),
    this->ShaderReplacementsString.c_str() + this->ShaderReplacementsString.length(), &root,
    nullptr);
  if (!success)
  {
    vtkGenericWarningMacro("Unable to parse the replacement Json string!");
    return;
  }
  bool isArray = root.isArray();
  size_t nbReplacements = isArray ? root.size() : 1;

  std::vector<std::tuple<vtkShader::Type, std::string, std::string>> replacements;
  for (size_t index = 0; index < nbReplacements; ++index)
  {
    const Json::Value& repl = isArray ? root[(int)index] : root;
    if (!repl.isMember("type"))
    {
      vtkErrorMacro("Syntax error in shader replacements: a type is required.");
      return;
    }
    std::string type = repl["type"].asString();
    vtkShader::Type shaderType = vtkShader::Unknown;
    if (type == "fragment")
    {
      shaderType = vtkShader::Fragment;
    }
    else if (type == "vertex")
    {
      shaderType = vtkShader::Vertex;
    }
    else if (type == "geometry")
    {
      shaderType = vtkShader::Geometry;
    }
    if (shaderType == vtkShader::Unknown)
    {
      vtkErrorMacro("Unknown shader type for replacement:" << type);
      return;
    }

    if (!repl.isMember("original"))
    {
      vtkErrorMacro("Syntax error in shader replacements: an original pattern is required.");
      return;
    }
    std::string original = repl["original"].asString();
    if (!repl.isMember("replacement"))
    {
      vtkErrorMacro("Syntax error in shader replacements: a replacement pattern is required.");
      return;
    }
    std::string replacement = repl["replacement"].asString();
    replacements.push_back(std::make_tuple(shaderType, original, replacement));
  }

  for (const auto& r : replacements)
  {
    switch (std::get<0>(r))
    {
      case vtkShader::Fragment:
        props->AddFragmentShaderReplacement(std::get<1>(r), true, std::get<2>(r), true);
        break;
      case vtkShader::Vertex:
        props->AddVertexShaderReplacement(std::get<1>(r), true, std::get<2>(r), true);
        break;
      case vtkShader::Geometry:
        props->AddGeometryShaderReplacement(std::get<1>(r), true, std::get<2>(r), true);
        break;
      default:
        assert(false && "unknown shader replacement type");
        break;
    }
  }
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetArrayIdNames(const char* pointArray, const char* cellArray)
{
  using namespace vtk::literals;

  if (auto* cpmapper = vtkCompositePolyDataMapper::SafeDownCast(this->Mapper))
  {
    cpmapper->SetPointIdArrayName(pointArray ? pointArray : "vtkOriginalPointIds");
    cpmapper->SetCellIdArrayName(cellArray ? cellArray : "vtkOriginalCellIds");
  }
  else if (auto* cgmapper = vtkCompositeCellGridMapper::SafeDownCast(this->Mapper))
  {
    cgmapper->SetPointIdAttributeName(pointArray ? pointArray : "vtkOriginalPointIds"_token);
    cgmapper->SetCellIdAttributeName(cellArray ? cellArray : "vtkOriginalCellIds"_token);
  }
}
