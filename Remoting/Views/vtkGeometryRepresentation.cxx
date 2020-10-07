/*=========================================================================

  Program:   ParaView
  Module:    vtkGeometryRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGeometryRepresentation.h"
#include "vtkGeometryRepresentationInternal.h"

#include "vtkAlgorithmOutput.h"
#include "vtkBoundingBox.h"
#include "vtkCallbackCommand.h"
#include "vtkCommand.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVConfig.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLogger.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPointData.h"
#include "vtkProcessModule.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSelection.h"
#include "vtkSelectionConverter.h"
#include "vtkSelectionNode.h"
#include "vtkShaderProperty.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtkUnstructuredGrid.h"

#if VTK_MODULE_ENABLE_VTK_RenderingRayTracing
#include "vtkOSPRayActorNode.h"
#endif

#include <vtk_jsoncpp.h>
#include <vtksys/SystemTools.hxx>

#include <memory>
#include <numeric>
#include <tuple>
#include <vector>

//*****************************************************************************
// This is used to convert a vtkPolyData to a vtkMultiBlockDataSet. If input is
// vtkMultiBlockDataSet, then this is simply a pass-through filter. This makes
// it easier to unify the code to select and render data by simply dealing with
// vtkMultiBlockDataSet always.
class vtkGeometryRepresentationMultiBlockMaker : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGeometryRepresentationMultiBlockMaker* New();
  vtkTypeMacro(vtkGeometryRepresentationMultiBlockMaker, vtkMultiBlockDataSetAlgorithm);

protected:
  int RequestData(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    vtkMultiBlockDataSet* inputMB = vtkMultiBlockDataSet::SafeDownCast(inputDO);
    vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::GetData(outputVector, 0);

    vtkInformation* infoNormals = this->GetInputArrayInformation(0);
    vtkInformation* infoTCoords = this->GetInputArrayInformation(1);
    vtkInformation* infoTangents = this->GetInputArrayInformation(2);

    std::string normalsName;
    std::string tcoordsName;
    std::string tangentsName;

    const char* normalField = infoNormals ? infoNormals->Get(vtkDataObject::FIELD_NAME()) : nullptr;
    const char* tcoordField = infoTCoords ? infoTCoords->Get(vtkDataObject::FIELD_NAME()) : nullptr;
    const char* tangentField =
      infoTangents ? infoTangents->Get(vtkDataObject::FIELD_NAME()) : nullptr;

    if (normalField)
    {
      normalsName = normalField;
    }
    if (tcoordField)
    {
      tcoordsName = tcoordField;
    }
    if (tangentField)
    {
      tangentsName = tangentField;
    }

    if (inputMB)
    {
      outputMB->ShallowCopy(inputMB);

      vtkNew<vtkDataObjectTreeIterator> iter;
      iter->SetDataSet(outputMB);
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        this->SetArrays(vtkDataSet::SafeDownCast(iter->GetCurrentDataObject()), normalsName,
          tcoordsName, tangentsName);
      }

      return 1;
    }

    auto clone = vtkSmartPointer<vtkDataObject>::Take(inputDO->NewInstance());
    clone->ShallowCopy(inputDO);
    outputMB->SetBlock(0, clone);

    this->SetArrays(vtkDataSet::SafeDownCast(clone), normalsName, tcoordsName, tangentsName);

    return 1;
  }

  void SetArrays(vtkDataSet* dataSet, const std::string& normal, const std::string& tcoord,
    const std::string& tangent)
  {
    if (dataSet)
    {
      if (!normal.empty())
      {
        dataSet->GetPointData()->SetActiveNormals(normal.c_str());
      }
      if (!tcoord.empty())
      {
        dataSet->GetPointData()->SetActiveTCoords(tcoord.c_str());
      }
      if (!tangent.empty())
      {
        dataSet->GetPointData()->SetActiveTangents(tangent.c_str());
      }
    }
  }

  int FillInputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
    return 1;
  }
};
vtkStandardNewMacro(vtkGeometryRepresentationMultiBlockMaker);

//*****************************************************************************

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

vtkGeometryRepresentation::vtkGeometryRepresentation()
{
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
  vtkCompositePolyDataMapper2* mapper = vtkCompositePolyDataMapper2::New();
  mapper->SetProcessIdArrayName("vtkProcessId");
  mapper->SetCompositeIdArrayName("vtkCompositeIndex");

  this->Mapper = mapper;

  this->SetArrayIdNames(nullptr, nullptr);

  this->LODMapper = vtkCompositePolyDataMapper2::New();
  this->Actor = vtkPVLODActor::New();
  this->Property = vtkProperty::New();

  // setup composite display attributes
  vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributes;
  vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)
    ->SetCompositeDataDisplayAttributes(compositeAttributes);

  vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributesLOD;
  vtkCompositePolyDataMapper2::SafeDownCast(this->LODMapper)
    ->SetCompositeDataDisplayAttributes(compositeAttributesLOD);

  vtkNew<vtkSelection> sel;
  this->Mapper->SetSelection(sel);

  this->RequestGhostCellsIfNeeded = true;
  this->RepeatTextures = true;
  this->InterpolateTextures = false;
  this->UseMipmapTextures = false;
  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->Representation = SURFACE;

  this->SuppressLOD = false;

  vtkMath::UninitializeBounds(this->VisibleDataBounds);

  this->SetupDefaults();

  this->PWF = NULL;

  this->UseDataPartitions = false;

  this->UseShaderReplacements = false;
  this->ShaderReplacementsString = "";
}

//----------------------------------------------------------------------------
vtkGeometryRepresentation::~vtkGeometryRepresentation()
{
  this->GeometryFilter->Delete();
  this->MultiBlockMaker->Delete();
  this->Decimator->Delete();
  this->LODOutlineFilter->Delete();
  this->Mapper->Delete();
  this->LODMapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetupDefaults()
{
  this->Decimator->SetLODFactor(0.5);

  this->LODOutlineFilter->SetUseOutline(1);

  vtkPVGeometryFilter* geomFilter = vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter);
  if (geomFilter)
  {
    geomFilter->SetUseOutline(0);
    geomFilter->SetTriangulate(0);
    geomFilter->SetNonlinearSubdivisionLevel(1);
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
    // provide the "geometry" to the view so the view can delivery it to the
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
    auto data = vtkPVView::GetDeliveredPiece(inInfo, this);
    // vtkLogF(INFO, "%p: %s", (void*)data, this->GetLogName().c_str());
    auto dataLOD = vtkPVView::GetDeliveredPieceLOD(inInfo, this);
    this->Mapper->SetInputDataObject(data);
    this->LODMapper->SetInputDataObject(dataLOD);

    // This is called just before the vtk-level render. In this pass, we simply
    // pick the correct rendering mode and rendering parameters.
    bool lod = this->SuppressLOD ? false : (inInfo->Has(vtkPVRenderView::USE_LOD()) == 1);
    this->Actor->SetEnableLOD(lod ? 1 : 0);
    this->UpdateColoringParameters();

    if (data && (this->BlockAttributeTime < data->GetMTime() || this->BlockAttrChanged))
    {
      this->UpdateBlockAttributes(this->Mapper);
      this->BlockAttributeTime.Modified();
      this->BlockAttrChanged = false;

      // This flag makes the following LOD render requests to also update the block
      // attribute state, if there were any changes.
      this->UpdateBlockAttrLOD = true;
    }

    if (lod && dataLOD && this->UpdateBlockAttrLOD)
    {
      this->UpdateBlockAttributes(this->LODMapper);
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
    // vtkLogF(INFO, "%s->RequestData", this->GetLogName().c_str());
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
      vtkAlgorithmOutput* aout = this->GetInternalOutputPort();
      vtkPVTrivialProducer* prod = vtkPVTrivialProducer::SafeDownCast(aout->GetProducer());
      if (prod)
      {
        prod->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
      }
    }
    this->GeometryFilter->SetInputConnection(this->GetInternalOutputPort());
  }
  else
  {
    vtkNew<vtkMultiBlockDataSet> placeholder;
    this->GeometryFilter->SetInputDataObject(0, placeholder);
  }

  // essential to re-execute geometry filter consistently on all ranks since it
  // does use parallel communication (see #19963).
  this->GeometryFilter->Modified();
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
  return NULL;
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
  vtkOpenGLPolyDataMapper* mapper = vtkOpenGLPolyDataMapper::SafeDownCast(this->Mapper);
  if (mapper)
  {
    mapper->SetVBOShiftScaleMethod(val);
  }
  mapper = vtkOpenGLPolyDataMapper::SafeDownCast(this->LODMapper);
  if (mapper)
  {
    mapper->SetVBOShiftScaleMethod(val);
  }
}

int vtkGeometryRepresentation::GetCoordinateShiftScaleMethod()
{
  vtkOpenGLPolyDataMapper* mapper = vtkOpenGLPolyDataMapper::SafeDownCast(this->Mapper);
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
        [](bool result, const std::pair<unsigned int, double>& apair) {
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
void vtkGeometryRepresentation::SetEdgeTint(double r, double g, double b)
{
  this->Property->SetEdgeTint(r, g, b);
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
  vtkInformation* info = this->Actor->GetPropertyKeys();
  info->Remove(vtkProp::GeneralTextureTransform());
  if (flip)
  {
    double mat[] = { 1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    info->Set(vtkProp::GeneralTextureTransform(), mat, 16);
  }
  this->Actor->Modified();
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
  if (vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetUseOutline(val);
  }

  // since geometry filter needs to execute, we need to mark the representation
  // modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetTriangulate(int val)
{
  if (vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetTriangulate(val);
  }

  // since geometry filter needs to execute, we need to mark the representation
  // modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetNonlinearSubdivisionLevel(int val)
{
  if (vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetNonlinearSubdivisionLevel(val);
  }

  // since geometry filter needs to execute, we need to mark the representation
  // modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetGenerateFeatureEdges(bool val)
{
  if (vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
  {
    vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetGenerateFeatureEdges(val);
  }

  // since geometry filter needs to execute, we need to mark the representation
  // modified.
  this->MarkModified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockVisibility(unsigned int index, bool visible)
{
  this->BlockVisibilities[index] = visible;
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::GetBlockVisibility(unsigned int index) const
{
  auto it = this->BlockVisibilities.find(index);
  if (it == this->BlockVisibilities.cend())
  {
    return true;
  }
  return it->second;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockVisibility(unsigned int index, bool)
{
  auto it = this->BlockVisibilities.find(index);
  if (it == this->BlockVisibilities.cend())
  {
    return;
  }
  this->BlockVisibilities.erase(it);
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockVisibilities()
{
  this->BlockVisibilities.clear();
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockColor(unsigned int index, double r, double g, double b)
{
  std::array<double, 3> color = { { r, g, b } };
  this->BlockColors[index] = color;
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockColor(unsigned int index, double* color)
{
  if (color)
  {
    this->SetBlockColor(index, color[0], color[1], color[2]);
  }
}

//----------------------------------------------------------------------------
double* vtkGeometryRepresentation::GetBlockColor(unsigned int index)
{
  auto it = this->BlockColors.find(index);
  if (it == this->BlockColors.cend())
  {
    return nullptr;
  }
  return it->second.data();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockColor(unsigned int index)
{
  auto it = this->BlockColors.find(index);
  if (it == this->BlockColors.cend())
  {
    return;
  }
  this->BlockColors.erase(it);
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockColors()
{
  this->BlockColors.clear();
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockOpacity(unsigned int index, double opacity)
{
  this->BlockOpacities[index] = opacity;
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockOpacity(unsigned int index, double* opacity)
{
  if (opacity)
  {
    this->SetBlockOpacity(index, *opacity);
  }
}

//----------------------------------------------------------------------------
double vtkGeometryRepresentation::GetBlockOpacity(unsigned int index)
{
  auto it = this->BlockOpacities.find(index);
  if (it == this->BlockOpacities.cend())
  {
    return 0.0;
  }
  return it->second;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockOpacity(unsigned int index)
{
  auto it = this->BlockOpacities.find(index);
  if (it == this->BlockOpacities.cend())
  {
    return;
  }
  this->BlockOpacities.erase(it);
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockOpacities()
{
  this->BlockOpacities.clear();
  this->BlockAttrChanged = true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::UpdateBlockAttributes(vtkMapper* mapper)
{
  auto cpm = vtkCompositePolyDataMapper2::SafeDownCast(mapper);
  if (!cpm)
  {
    vtkErrorMacro(<< "Invalid mapper!");
    return;
  }

  cpm->RemoveBlockVisibilities();
  for (auto const& item : this->BlockVisibilities)
  {
    cpm->SetBlockVisibility(item.first, item.second);
  }

  cpm->RemoveBlockColors();
  for (auto const& item : this->BlockColors)
  {
    auto& arr = item.second;
    double color[3] = { arr[0], arr[1], arr[2] };
    cpm->SetBlockColor(item.first, color);
  }

  cpm->RemoveBlockOpacities();
  for (auto const& item : this->BlockOpacities)
  {
    cpm->SetBlockOpacity(item.first, item.second);
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
    vtkDataObject* dataObject = this->MultiBlockMaker->GetOutputDataObject(0);
    vtkNew<vtkCompositeDataDisplayAttributes> cdAttributes;
    // If the input data is a composite dataset, use the currently set values for block
    // visibility rather than the cached ones from the last render.  This must be computed
    // in the REQUEST_UPDATE pass but the data is only copied to the mapper in the
    // REQUEST_RENDER pass.  This constructs a dummy vtkCompositeDataDisplayAttributes
    // with only the visibilities set and calls the helper function to compute the visible
    // bounds with that.
    if (vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(dataObject))
    {
      auto iter = vtkSmartPointer<vtkCompositeDataIterator>::Take(cd->NewIterator());
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        auto visible = this->BlockVisibilities.find(iter->GetCurrentFlatIndex());
        if (visible != this->BlockVisibilities.end())
        {
          cdAttributes->SetBlockVisibility(iter->GetCurrentDataObject(), visible->second);
        }
      }
    }
    this->GetBounds(dataObject, this->VisibleDataBounds, cdAttributes);
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
  if (strcmp(replacementsString, this->ShaderReplacementsString.c_str()))
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

  if (!this->UseShaderReplacements || this->ShaderReplacementsString == "")
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

  std::vector<std::tuple<vtkShader::Type, std::string, std::string> > replacements;
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
  vtkCompositePolyDataMapper2* mapper = vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper);
  mapper->SetPointIdArrayName(pointArray ? pointArray : "vtkOriginalPointIds");
  mapper->SetCellIdArrayName(cellArray ? cellArray : "vtkOriginalCellIds");
}
