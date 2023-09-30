// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkCellGrid.h"
#include "vtkCellGridComputeSurface.h"
#include "vtkCompositeCellGridMapper.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkDataAssembly.h"
#include "vtkDataAssemblyUtilities.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkDataObjectTreeRange.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVLogger.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkRenderer.h"
#include "vtkRenderingCellGrid.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTexture.h"

#include <vtk_jsoncpp.h>
#include <vtksys/SystemTools.hxx>

#include <memory>
#include <numeric>
#include <tuple>
#include <vector>

//*****************************************************************************
// This is used to convert a vtkPolyData to a vtkPartitionedDataSetCollection.
// If the input is a vtkPartitionedDataSetCollection, then this is simply a
// pass-through filter. This makes it easier to unify the code to select and
// render data by simply dealing with vtkPartitionedDataSetCollection always.
class vtkCellGridRepresentationMultiBlockMaker : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkCellGridRepresentationMultiBlockMaker* New();
  vtkTypeMacro(vtkCellGridRepresentationMultiBlockMaker, vtkPartitionedDataSetCollectionAlgorithm);

protected:
  int RequestData(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override
  {
    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    auto* inputPDC = vtkPartitionedDataSetCollection::SafeDownCast(inputDO);
    auto* outputPDC = vtkPartitionedDataSetCollection::GetData(outputVector, 0);

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

    if (inputPDC)
    {
      outputPDC->ShallowCopy(inputPDC);

      vtkNew<vtkDataObjectTreeIterator> iter;
      iter->SetDataSet(outputPDC);
      iter->SkipEmptyNodesOn();
      iter->VisitOnlyLeavesOn();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        this->SetArrays(vtkCellGrid::SafeDownCast(iter->GetCurrentDataObject()), normalsName,
          tcoordsName, tangentsName);
      }

      return 1;
    }

    auto clone = vtkSmartPointer<vtkDataObject>::Take(inputDO->NewInstance());
    clone->ShallowCopy(inputDO);
    vtkNew<vtkPartitionedDataSet> partition;
    partition->SetPartition(0, clone);
    outputPDC->SetPartitionedDataSet(0, partition);
    this->SetArrays(vtkCellGrid::SafeDownCast(clone), normalsName, tcoordsName, tangentsName);
    // if we created a MB out of a non-composite dataset, we add this array to
    // make it possible for `PopulateBlockAttributes` to be aware of that.

    vtkNew<vtkIntArray> marker;
    marker->SetName("vtkCellGridRepresentationMultiBlockMaker");
    outputPDC->GetFieldData()->AddArray(marker);
    return 1;
  }

  void SetArrays(vtkCellGrid* dataSet, const std::string& normal, const std::string& tcoord,
    const std::string& tangent)
  {
    if (dataSet)
    {
      if (!normal.empty())
      {
        dataSet->GetAttributes(vtkDataObject::POINT)->SetActiveNormals(normal.c_str());
      }
      if (!tcoord.empty())
      {
        dataSet->GetAttributes(vtkDataObject::POINT)->SetActiveTCoords(tcoord.c_str());
      }
      if (!tangent.empty())
      {
        dataSet->GetAttributes(vtkDataObject::POINT)->SetActiveTangents(tangent.c_str());
      }
    }
  }

  int FillInputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");
    return 1;
  }
};
vtkStandardNewMacro(vtkCellGridRepresentationMultiBlockMaker);

//*****************************************************************************

vtkStandardNewMacro(vtkCellGridRepresentation);

vtkCellGridRepresentation::vtkCellGridRepresentation()
{
  vtkRenderingCellGrid::RegisterCellsAndResponders();
  this->GeometryFilter->Delete();
  this->GeometryFilter = nullptr;
  this->MultiBlockMaker->Delete();
  this->MultiBlockMaker = nullptr;

  this->SetupDefaults();
}

vtkCellGridRepresentation::~vtkCellGridRepresentation() = default;

void vtkCellGridRepresentation::SetupDefaults()
{
  // delete vtkCompositePolyDataMapper created by vtkGeometryRepresentation
  this->GeometryFilter = vtkCellGridComputeSurface::New();
  this->MultiBlockMaker = vtkCellGridRepresentationMultiBlockMaker::New();
  this->LODOutlineFilter->Delete();
  this->LODOutlineFilter = vtkPVGeometryFilter::New();

  // connect progress bar
  this->GeometryFilter->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkCellGridRepresentation::HandleGeometryRepresentationProgress);
  this->MultiBlockMaker->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkCellGridRepresentation::HandleGeometryRepresentationProgress);
#if 0
  this->Decimator->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkCellGridRepresentation::HandleGeometryRepresentationProgress);
#endif // 0
  this->LODOutlineFilter->AddObserver(vtkCommand::ProgressEvent, this,
    &vtkCellGridRepresentation::HandleGeometryRepresentationProgress);

  // setup the selection mapper so that we don't need to make any selection
  // conversions after rendering.
  vtkCompositeCellGridMapper* mapper = vtkCompositeCellGridMapper::New();
  mapper->SetProcessIdArrayName("vtkProcessId");
  mapper->SetCompositeIdArrayName("vtkCompositeIndex");

  // delete vtkCompositePolyDataMapper created by vtkGeometryRepresentation
  this->Mapper->Delete();
  this->Mapper = mapper;

  this->SetArrayIdNames(nullptr, nullptr);

  // delete vtkCompositePolyDataMapper created by vtkGeometryRepresentation
  this->LODMapper->Delete();
  this->LODMapper = vtkCompositeCellGridMapper::New();
  this->Actor->Delete();
  this->Actor = vtkPVLODActor::New();
  this->Property->Delete();
  this->Property = vtkProperty::New();

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

  // setup composite display attributes
  if (auto cpdm = vtkCompositeCellGridMapper::SafeDownCast(this->Mapper))
  {
    vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributes;
    cpdm->SetCompositeDataDisplayAttributes(compositeAttributes);
  }

  if (auto cpdm = vtkCompositeCellGridMapper::SafeDownCast(this->LODMapper))
  {
    vtkNew<vtkCompositeDataDisplayAttributes> compositeAttributesLOD;
    cpdm->SetCompositeDataDisplayAttributes(compositeAttributesLOD);
  }

  vtkNew<vtkSelection> sel;
  this->Mapper->SetSelection(sel);

  // this->Decimator->SetLODFactor(0.5);

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

int vtkCellGridRepresentation::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPartitionedDataSetCollection");

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

int vtkCellGridRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->vtkPVDataRepresentation::ProcessViewRequest(request_type, inInfo, outInfo))
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
      // Always use outline in LOD mode.
      this->LODOutlineFilter->SetInputDataObject(data);
      this->LODOutlineFilter->Update();
      // Pass along the LOD geometry to the view so that it can deliver it to
      // the rendering node as and when needed.
      vtkPVView::SetPieceLOD(inInfo, this, this->LODOutlineFilter->GetOutputDataObject(0));
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
      if (auto cmapper = vtkCompositeCellGridMapper::SafeDownCast(this->Mapper))
      {
        this->PopulateBlockAttributes(cmapper->GetCompositeDataDisplayAttributes(), data);
      }
      this->BlockAttributeTime.Modified();
      this->BlockAttrChanged = false;

      // This flag makes the following LOD render requests to also update the block
      // attribute state, if there were any changes.
      this->UpdateBlockAttrLOD = true;
    }

    if (lod && dataLOD && this->UpdateBlockAttrLOD)
    {
      if (auto cmapper = vtkCompositeCellGridMapper::SafeDownCast(this->LODMapper))
      {
        this->PopulateBlockAttributes(cmapper->GetCompositeDataDisplayAttributes(), dataLOD);
      }
      this->UpdateBlockAttrLOD = false;
    }
  }

  return 1;
}

int vtkCellGridRepresentation::RequestData(
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
    vtkNew<vtkPartitionedDataSetCollection> placeholder;
    this->GeometryFilter->SetInputDataObject(0, placeholder);
  }

  // essential to re-execute geometry filter consistently on all ranks since it
  // does use parallel communication (see #19963).
  this->GeometryFilter->Modified();
  this->MultiBlockMaker->Update();
  return this->Superclass::RequestData(request, inputVector, outputVector);
}

bool vtkCellGridRepresentation::AddToView(vtkView* view)
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

bool vtkCellGridRepresentation::RemoveFromView(vtkView* view)
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

void vtkCellGridRepresentation::UpdateColoringParameters()
{
  return this->Superclass::UpdateColoringParameters();
}

void vtkCellGridRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
}

bool vtkCellGridRepresentation::NeedsOrderedCompositing()
{
  // One would think simply calling `vtkActor::HasTranslucentPolygonalGeometry`
  // should do the trick, however that method relies on the mapper's input
  // having up-to-date data. vtkCellGridRepresentation needs to determine
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

void vtkCellGridRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
