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

#include "vtkAlgorithmOutput.h"
#include "vtkCommand.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOrderedCompositeDistributor.h"
#include "vtkPKdTree.h"
#include "vtkProperty.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVGeometryFilter.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkPVTrivialProducer.h"
#include "vtkPVUpdateSuppressor.h"
#include "vtkQuadricClustering.h"
#include "vtkRenderer.h"
#include "vtkSelectionConverter.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkShadowMapBakerPass.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredDataDeliveryFilter.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

//*****************************************************************************
class vtkGeometryRepresentationMultiBlockMaker : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGeometryRepresentationMultiBlockMaker* New();
  vtkTypeMacro(vtkGeometryRepresentationMultiBlockMaker, vtkMultiBlockDataSetAlgorithm);
protected:
  virtual int RequestData(vtkInformation *,
    vtkInformationVector ** inputVector, vtkInformationVector *outputVector)
    {
    vtkMultiBlockDataSet* inputMB =
      vtkMultiBlockDataSet::GetData(inputVector[0], 0);
    vtkMultiBlockDataSet* outputMB =
      vtkMultiBlockDataSet::GetData(outputVector, 0);
    if (inputMB)
      {
      outputMB->ShallowCopy(inputMB);
      return 1;
      }

    vtkDataObject* inputDO = vtkDataObject::GetData(inputVector[0], 0);
    vtkDataObject* clone = inputDO->NewInstance();
    clone->ShallowCopy(inputDO);
    outputMB->SetBlock(0, clone);
    clone->Delete();
    return 1;
    }

  virtual int FillInputPortInformation(int, vtkInformation *info)
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
vtkGeometryRepresentation::vtkGeometryRepresentation()
{
  this->GeometryFilter = vtkPVGeometryFilter::New();
  this->CacheKeeper = vtkPVCacheKeeper::New();
  this->MultiBlockMaker = vtkGeometryRepresentationMultiBlockMaker::New();
  this->Decimator = vtkQuadricClustering::New();
  this->Mapper = vtkCompositePolyDataMapper2::New();
  this->LODMapper = vtkCompositePolyDataMapper2::New();
  this->Actor = vtkPVLODActor::New();
  this->Property = vtkProperty::New();
  this->DeliveryFilter = vtkUnstructuredDataDeliveryFilter::New();
  this->LODDeliveryFilter = vtkUnstructuredDataDeliveryFilter::New();
  this->Distributor = vtkOrderedCompositeDistributor::New();
  this->UpdateSuppressor = vtkPVUpdateSuppressor::New();
  this->LODUpdateSuppressor = vtkPVUpdateSuppressor::New();
  this->DeliverySuppressor = vtkPVUpdateSuppressor::New();
  this->LODDeliverySuppressor = vtkPVUpdateSuppressor::New();
  this->RequestGhostCellsIfNeeded = true;

  this->ColorArrayName = 0;
  this->ColorAttributeType = VTK_SCALAR_MODE_DEFAULT;
  this->Ambient = 0.0;
  this->Diffuse = 1.0;
  this->Specular = 0.0;
  this->Representation = SURFACE;

  this->SuppressLOD = false;
  this->DebugString = 0;
  this->SetDebugString(this->GetClassName());

  this->AllowSpecularHighlightingWithScalarColoring = false;

  this->SetupDefaults();
}

//----------------------------------------------------------------------------
vtkGeometryRepresentation::~vtkGeometryRepresentation()
{
  this->SetDebugString(0);
  this->CacheKeeper->Delete();
  this->GeometryFilter->Delete();
  this->MultiBlockMaker->Delete();
  this->Decimator->Delete();
  this->Mapper->Delete();
  this->LODMapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->DeliveryFilter->Delete();
  this->LODDeliveryFilter->Delete();
  this->Distributor->Delete();
  this->UpdateSuppressor->Delete();
  this->LODUpdateSuppressor->Delete();
  this->DeliverySuppressor->Delete();
  this->LODDeliverySuppressor->Delete();
  this->SetColorArrayName(0);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetupDefaults()
{
  this->Decimator->SetUseInputPoints(1);
  this->Decimator->SetCopyCellData(1);
  this->Decimator->SetUseInternalTriangles(0);
  this->Decimator->SetNumberOfDivisions(10, 10, 10);
  this->LODDeliveryFilter->SetLODMode(true); // tell the filter that it is
                                             // connected to the LOD pipeline.

  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetUseOutline(0);
  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetNonlinearSubdivisionLevel(1);
  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetPassThroughCellIds(1);
  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetPassThroughPointIds(1);

  this->DeliveryFilter->SetOutputDataType(VTK_MULTIBLOCK_DATA_SET);
  this->LODDeliveryFilter->SetOutputDataType(VTK_MULTIBLOCK_DATA_SET);

  this->DeliverySuppressor->SetInputConnection(this->DeliveryFilter->GetOutputPort());
  this->LODDeliverySuppressor->SetInputConnection(this->LODDeliveryFilter->GetOutputPort());

  this->Distributor->SetController(vtkMultiProcessController::GetGlobalController());
  this->Distributor->SetInputConnection(0, this->DeliverySuppressor->GetOutputPort());
  this->Distributor->SetPassThrough(1);

  this->MultiBlockMaker->SetInputConnection(this->GeometryFilter->GetOutputPort());
  this->CacheKeeper->SetInputConnection(this->MultiBlockMaker->GetOutputPort());
  this->Decimator->SetInputConnection(this->CacheKeeper->GetOutputPort());

  this->UpdateSuppressor->SetInputConnection(this->Distributor->GetOutputPort());
  this->LODUpdateSuppressor->SetInputConnection(this->LODDeliverySuppressor->GetOutputPort());

  this->Mapper->SetInputConnection(this->UpdateSuppressor->GetOutputPort());
  this->LODMapper->SetInputConnection(this->LODUpdateSuppressor->GetOutputPort());

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
int vtkGeometryRepresentation::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->GetVisibility())
    {
    return false;
    }

  if (request_type == vtkPVView::REQUEST_INFORMATION())
    {
    this->GenerateMetaData(inInfo, outInfo);
    }
  else if (request_type == vtkPVView::REQUEST_PREPARE_FOR_RENDER())
    {
    // In REQUEST_PREPARE_FOR_RENDER, we need to ensure all our data-deliver
    // filters have their states updated as requested by the view.

    // this is where we will look to see on what nodes are we going to render and
    // render set that up.
    bool lod = this->SuppressLOD? false :
      (inInfo->Has(vtkPVRenderView::USE_LOD()) == 1);
    if (lod)
      {
      if (inInfo->Has(vtkPVRenderView::LOD_RESOLUTION()))
        {
        int division = static_cast<int>(150 *
          inInfo->Get(vtkPVRenderView::LOD_RESOLUTION())) + 10;
        this->Decimator->SetNumberOfDivisions(division, division, division);
        }
      this->LODDeliveryFilter->ProcessViewRequest(inInfo);
      if (this->LODDeliverySuppressor->GetForcedUpdateTimeStamp() <
        this->LODDeliveryFilter->GetMTime())
        {
        outInfo->Set(vtkPVRenderView::NEEDS_DELIVERY(), 1);
        }
      }
    else
      {
      this->DeliveryFilter->ProcessViewRequest(inInfo);
      if (this->DeliverySuppressor->GetForcedUpdateTimeStamp() <
        this->DeliveryFilter->GetMTime())
        {
        outInfo->Set(vtkPVRenderView::NEEDS_DELIVERY(), 1);
        }
      }
    this->Actor->SetEnableLOD(lod? 1 : 0);
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

    this->UpdateColoringParameters();
    if (this->Actor->GetEnableLOD())
      {
      this->LODUpdateSuppressor->ForceUpdate();
      }
    else
      {
      this->UpdateSuppressor->ForceUpdate();
      }
    }

  return this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::DoRequestGhostCells(vtkInformation* info)
{
  vtkMultiProcessController* controller =
    vtkMultiProcessController::GetGlobalController();
  if (controller == NULL || controller->GetNumberOfProcesses() <= 1)
    {
    return false;
    }

  if (vtkUnstructuredGrid::GetData(info) != NULL ||
    vtkCompositeDataSet::GetData(info) != NULL)
    {
    // ensure that there's no WholeExtent to ensure
    // that this UG was never born out of a structured dataset.
    bool has_whole_extent = (info->Has(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()) != 0);
    if (!has_whole_extent)
      {
      //cout << "Need ghosts" << endl;
      return true;
      }
    }

  return false;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  this->Superclass::RequestUpdateExtent(request, inputVector, outputVector);

  // ensure that the ghost-level information is setup correctly to avoid
  // internal faces for unstructured grids.
  for (int cc=0; cc < this->GetNumberOfInputPorts(); cc++)
    {
    for (int kk=0; kk < inputVector[cc]->GetNumberOfInformationObjects(); kk++)
      {
      vtkInformation* inInfo = inputVector[cc]->GetInformationObject(kk);

      vtkStreamingDemandDrivenPipeline* sddp =
        vtkStreamingDemandDrivenPipeline::SafeDownCast(this->GetExecutive());
      int ghostLevels = sddp->GetUpdateGhostLevel(inInfo);
      if (this->RequestGhostCellsIfNeeded &&
        vtkGeometryRepresentation::DoRequestGhostCells(inInfo))
        {
        ghostLevels++;
        }
      sddp->SetUpdateGhostLevel(inInfo, ghostLevels);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // cout << this << ":" << this->DebugString << ":RequestData" << endl;

  // mark delivery filters modified.
  this->DeliveryFilter->Modified();
  this->LODDeliveryFilter->Modified();
  this->Distributor->Modified();


  // Pass caching information to the cache keeper.
  this->CacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->CacheKeeper->SetCacheTime(this->GetCacheKey());

  if (inputVector[0]->GetNumberOfInformationObjects()==1)
    {
    vtkInformation* inInfo =
      inputVector[0]->GetInformationObject(0);
    if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
      {
      vtkAlgorithmOutput* aout =
        this->GetInternalOutputPort();
      vtkPVTrivialProducer* prod = vtkPVTrivialProducer::SafeDownCast(
        aout->GetProducer());
      if (prod)
        {
        prod->SetWholeExtent(inInfo->Get(
                               vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
        }
      }
    this->GeometryFilter->SetInputConnection(
      this->GetInternalOutputPort());
    this->CacheKeeper->Update();
    this->DeliveryFilter->SetInputConnection(
      this->CacheKeeper->GetOutputPort());
    this->LODDeliveryFilter->SetInputConnection(
      this->Decimator->GetOutputPort());
    }
  else
    {
    this->DeliveryFilter->RemoveAllInputs();
    this->LODDeliveryFilter->RemoveAllInputs();
    }

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::IsCached(double cache_key)
{
  return this->CacheKeeper->IsCached(cache_key);
}

//----------------------------------------------------------------------------
vtkDataObject* vtkGeometryRepresentation::GetRenderedDataObject(int port)
{
  // cout << this << ":" << this->DebugString << ":GetRenderedDataObject" << endl;
  (void) port;
  if (this->GeometryFilter->GetNumberOfInputConnections(0) > 0)
    {
    return this->CacheKeeper->GetOutputDataObject(0);
    }
  return NULL;
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::GenerateMetaData(vtkInformation*,
  vtkInformation* outInfo)
{
  if (this->GeometryFilter->GetNumberOfInputConnections(0) > 0)
    {
    vtkDataObject* geom = this->GeometryFilter->GetOutputDataObject(0);
    if (geom)
      {
      outInfo->Set(vtkPVRenderView::GEOMETRY_SIZE(),geom->GetActualMemorySize());
      }
    }

  outInfo->Set(vtkPVRenderView::REDISTRIBUTABLE_DATA_PRODUCER(),
    this->DeliveryFilter);
  if (this->Actor->GetProperty()->GetOpacity() < 1.0)
    {
    outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::MarkModified()
{
  //cout << this << ":" << this->DebugString << ":MarkModified" << endl;
  if (!this->GetUseCache())
    {
    // Cleanup caches when not using cache.
    this->CacheKeeper->RemoveAllCaches();
    }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::AddToView(vtkView* view)
{
  // FIXME: Need generic view API to add props.
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);
    return true;
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
    return true;
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
void vtkGeometryRepresentation::UpdateColoringParameters()
{
  bool using_scalar_coloring = false;
  if (this->ColorArrayName && this->ColorArrayName[0])
    {
    this->Mapper->SetScalarVisibility(1);
    this->LODMapper->SetScalarVisibility(1);
    this->Mapper->SelectColorArray(this->ColorArrayName);
    this->LODMapper->SelectColorArray(this->ColorArrayName);
    this->Mapper->SetUseLookupTableScalarRange(1);
    this->LODMapper->SetUseLookupTableScalarRange(1);
    switch (this->ColorAttributeType)
      {
    case CELL_DATA:
      this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
      break;

    case POINT_DATA:
    default:
      this->Mapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      this->LODMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
      break;
      }
    using_scalar_coloring = true;
    }
  else
    {
    this->Mapper->SetScalarVisibility(0);
    this->LODMapper->SetScalarVisibility(0);
    const char* null = NULL;
    this->Mapper->SelectColorArray(null);
    this->LODMapper->SelectColorArray(null);
    }

  // Adjust material properties.
  double diffuse = this->Diffuse;
  double specular = this->Specular;
  double ambient = this->Ambient;

  if (this->Representation != SURFACE &&
    this->Representation != SURFACE_WITH_EDGES)
    {
    diffuse = 0.0;
    ambient = 1.0;
    specular = 0.0;
    }
  else if (using_scalar_coloring && !this->AllowSpecularHighlightingWithScalarColoring)
    {
    // Disable specular highlighting if coloring by scalars.
    specular = 0.0;
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

  // Update shadow map properties, in case we are using shadow maps.
  if (this->Representation == SURFACE ||
    this->Representation == SURFACE_WITH_EDGES)
    {
    // just add these keys, their values don't matter.
    this->Actor->GetPropertyKeys()->Set(vtkShadowMapBakerPass::OCCLUDER(), 0);
    this->Actor->GetPropertyKeys()->Set(vtkShadowMapBakerPass::RECEIVER(), 0);
    }
  else
    {
    this->Actor->GetPropertyKeys()->Set(vtkShadowMapBakerPass::OCCLUDER(), 0);
    this->Actor->GetPropertyKeys()->Remove(vtkShadowMapBakerPass::RECEIVER());
    }
}

//----------------------------------------------------------------------------
vtkSelection* vtkGeometryRepresentation::ConvertSelection(
  vtkView* _view, vtkSelection* selection)
{
  vtkPVRenderView* view = vtkPVRenderView::SafeDownCast(_view);
  // if this->GeometryFilter has 0 inputs, it means we don't have any valid
  // input data on this process, so we can't convert the selection.
  if (!view ||
    this->GeometryFilter->GetNumberOfInputConnections(0) == 0)
    {
    return this->Superclass::ConvertSelection(_view, selection);
    }

  vtkSelection* newInput = vtkSelection::New();

  // locate any selection nodes which belong to this representation.
  for (unsigned int cc=0; cc < selection->GetNumberOfNodes(); cc++)
    {
    vtkSelectionNode* node = selection->GetNode(cc);
    vtkProp* prop = NULL;
    if (node->GetProperties()->Has(vtkSelectionNode::PROP()))
      {
      prop = vtkProp::SafeDownCast(node->GetProperties()->Get(vtkSelectionNode::PROP()));
      }

    if (prop == this->GetRenderedProp())
      {
      newInput->AddNode(node);
      node->GetProperties()->Set(vtkSelectionNode::SOURCE(),
        this->GeometryFilter);
      }
    }

  if (newInput->GetNumberOfNodes() == 0)
    {
    newInput->Delete();
    return selection;
    }

  vtkSelection* output = vtkSelection::New();
  vtkSelectionConverter* convertor = vtkSelectionConverter::New();
  convertor->Convert(newInput, output, 0);
  convertor->Delete();
  newInput->Delete();
  return output;
}

void vtkGeometryRepresentation::SetAllowSpecularHighlightingWithScalarColoring(int allow)
{
  this->AllowSpecularHighlightingWithScalarColoring = allow > 0 ? true : false;
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetVisibility(bool val)
{
  this->Actor->SetVisibility(val);
  this->Superclass::SetVisibility(val);
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
  this->Mapper->SetColorMode(val);
  this->LODMapper->SetColorMode(val);
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
void vtkGeometryRepresentation::SetTexture(vtkTexture* val)
{
  this->Actor->SetTexture(val);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetUseOutline(int val)
{
  if (vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
    {
    vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetUseOutline(val);
    }
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetNonlinearSubdivisionLevel(int val)
{
  if (vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter))
    {
    vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetNonlinearSubdivisionLevel(val);
    }
  this->Modified();
  this->DeliveryFilter->Modified();
  this->LODDeliveryFilter->Modified();
}
