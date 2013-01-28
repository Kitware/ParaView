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
#include "vtkBoundingBox.h"
#include "vtkCommand.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkHardwareSelectionPolyDataPainter.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
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
#include "vtkTransform.h"
#include "vtkUnstructuredGrid.h"

#include <vtksys/SystemTools.hxx>

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
  this->LODOutlineFilter = vtkPVGeometryFilter::New();

  // setup the selection mapper so that we don't need to make any selection
  // conversions after rendering.
  vtkCompositePolyDataMapper2* mapper = vtkCompositePolyDataMapper2::New();
  vtkHardwareSelectionPolyDataPainter* selPainter =
    vtkHardwareSelectionPolyDataPainter::SafeDownCast(
      mapper->GetSelectionPainter()->GetDelegatePainter());
  selPainter->SetPointIdArrayName("vtkOriginalPointIds");
  selPainter->SetCellIdArrayName("vtkOriginalCellIds");
  selPainter->SetProcessIdArrayName("vtkProcessId");
  selPainter->SetCompositeIdArrayName("vtkCompositeIndex");

  this->Mapper = mapper;
  this->LODMapper = vtkCompositePolyDataMapper2::New();
  this->Actor = vtkPVLODActor::New();
  this->Property = vtkProperty::New();

  // setup composite display attributes
  vtkCompositeDataDisplayAttributes *compositeAttributes =
    vtkCompositeDataDisplayAttributes::New();
  vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)->
    SetCompositeDataDisplayAttributes(compositeAttributes);
  vtkCompositePolyDataMapper2::SafeDownCast(this->LODMapper)->
    SetCompositeDataDisplayAttributes(compositeAttributes);
  compositeAttributes->Delete();

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

  vtkMath::UninitializeBounds(this->DataBounds);

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
  this->LODOutlineFilter->Delete();
  this->Mapper->Delete();
  this->LODMapper->Delete();
  this->Actor->Delete();
  this->Property->Delete();
  this->SetColorArrayName(0);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetupDefaults()
{
  this->Decimator->SetUseInputPoints(1);
  this->Decimator->SetCopyCellData(1);
  this->Decimator->SetUseInternalTriangles(0);
  this->Decimator->SetNumberOfDivisions(10, 10, 10);
  
  this->LODOutlineFilter->SetUseOutline(1);

  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetUseOutline(0);
  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetNonlinearSubdivisionLevel(1);
  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetPassThroughCellIds(1);
  vtkPVGeometryFilter::SafeDownCast(this->GeometryFilter)->SetPassThroughPointIds(1);

  this->MultiBlockMaker->SetInputConnection(this->GeometryFilter->GetOutputPort());
  this->CacheKeeper->SetInputConnection(this->MultiBlockMaker->GetOutputPort());
  this->Decimator->SetInputConnection(this->CacheKeeper->GetOutputPort());
  this->LODOutlineFilter->SetInputConnection(this->CacheKeeper->GetOutputPort());

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

  // Saying INPUT_IS_OPTIONAL() is essential, since representations don't have
  // any inputs on client-side (in client-server, client-render-server mode) and
  // render-server-side (in client-render-server mode).
  info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type,
  vtkInformation* inInfo, vtkInformation* outInfo)
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
    // When this process doesn't have any valid input, the cache-keeper is setup
    // to provide a place-holder dataset of the right type. This is essential
    // since the vtkPVRenderView uses the type specified to decide on the
    // delivery mechanism, among other things.
    vtkPVRenderView::SetPiece(inInfo, this,
      this->CacheKeeper->GetOutputDataObject(0));

    // Since we are rendering polydata, it can be redistributed when ordered
    // compositing is needed. So let the view know that it can feel free to
    // redistribute data as and when needed.
    vtkPVRenderView::MarkAsRedistributable(inInfo, this);

    // Tell the view if this representation needs ordered compositing. We need
    // ordered compositing when rendering translucent geometry. 
    if (this->Actor->HasTranslucentPolygonalGeometry())
      {
      // We need to extend this condition to consider translucent LUTs once we
      // start supporting them,

      outInfo->Set(vtkPVRenderView::NEED_ORDERED_COMPOSITING(), 1);
      }

    // Finally, let the view know about the geometry bounds. The view uses this
    // information for resetting camera and clip planes. Since this
    // representation allows users to transform the geometry, we need to ensure
    // that the bounds we report include the transformation as well.
    vtkNew<vtkMatrix4x4> matrix;
    this->Actor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, this->DataBounds,
      matrix.GetPointer());
    }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
    {
    // Called to generate and provide the LOD data to the view.
    // If SuppressLOD is true, we tell the view we have no LOD data to provide,
    // otherwise we provide the decimated data.
    if (!this->SuppressLOD)
      {
      if (inInfo->Has(vtkPVRenderView::USE_OUTLINE_FOR_LOD()))
        {
        // HACK to ensure that when Decimator is next employed, it delivers a
        // new geometry.
        this->Decimator->Modified();

        this->LODOutlineFilter->Update();
        // Pass along the LOD geometry to the view so that it can deliver it to
        // the rendering node as and when needed.
        vtkPVRenderView::SetPieceLOD(inInfo, this, 
          this->LODOutlineFilter->GetOutputDataObject(0));
        }
      else
        {
        // HACK to ensure that when Decimator is next employed, it delivers a
        // new geometry.
        this->LODOutlineFilter->Modified();

        if (inInfo->Has(vtkPVRenderView::LOD_RESOLUTION()))
          {
          int division = static_cast<int>(150 *
            inInfo->Get(vtkPVRenderView::LOD_RESOLUTION())) + 10;
          this->Decimator->SetNumberOfDivisions(division, division, division);
          }

        this->Decimator->Update();

        // Pass along the LOD geometry to the view so that it can deliver it to
        // the rendering node as and when needed.
        vtkPVRenderView::SetPieceLOD(inInfo, this, 
          this->Decimator->GetOutputDataObject(0));
        }
      }
    }
  else if (request_type == vtkPVView::REQUEST_RENDER())
    {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this);
    vtkAlgorithmOutput* producerPortLOD = vtkPVRenderView::GetPieceProducerLOD(inInfo, this);
    this->Mapper->SetInputConnection(0, producerPort);
    this->LODMapper->SetInputConnection(0, producerPortLOD);

    // This is called just before the vtk-level render. In this pass, we simply
    // pick the correct rendering mode and rendering parameters.
    bool lod = this->SuppressLOD? false :
      (inInfo->Has(vtkPVRenderView::USE_LOD()) == 1);
    this->Actor->SetEnableLOD(lod? 1 : 0);
    this->UpdateColoringParameters();
    }

  return 1;
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

      int ghostLevels = vtkStreamingDemandDrivenPipeline::GetUpdateGhostLevel(inInfo);
      if (this->RequestGhostCellsIfNeeded &&
        vtkGeometryRepresentation::DoRequestGhostCells(inInfo))
        {
        ghostLevels++;
        }
      vtkStreamingDemandDrivenPipeline::SetUpdateGhostLevel(inInfo, ghostLevels);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkGeometryRepresentation::RequestData(vtkInformation* request,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // cout << this << ":" << this->DebugString << ":RequestData" << endl;

  vtkMath::UninitializeBounds(this->DataBounds);

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
    }
  else
    {
    vtkNew<vtkMultiBlockDataSet> placeholder;
    vtkPVGeometryFilter::SafeDownCast(
      this->GeometryFilter)->SetInputData(0, placeholder.GetPointer());
    }
  this->CacheKeeper->Update();

  // Determine data bounds.
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(
    this->CacheKeeper->GetOutputDataObject(0));
  if (cd)
    {
    vtkBoundingBox bbox;
    vtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataSet* ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
      if (ds)
        {
        bbox.AddBounds(ds->GetBounds());
        }
      }
    iter->Delete();
    if (bbox.IsValid())
      {
      bbox.GetBounds(this->DataBounds);
      }
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
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
    {
    rview->GetRenderer()->AddActor(this->Actor);

    // Indicate that this is prop that we are rendering when hardware selection
    // is enabled.
    rview->RegisterPropForHardwareSelection(this, this->GetRenderedProp());
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
    rview->UnRegisterPropForHardwareSelection(this, this->GetRenderedProp());
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
void vtkGeometryRepresentation::SetUserTransform(const double matrix[16])
{
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix);
  this->Actor->SetUserTransform(transform.GetPointer());
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
#if !defined(VTK_LEGACY_REMOVE)
bool vtkGeometryRepresentation::GenerateMetaData(vtkInformation*,
  vtkInformation*)
{
  vtkWarningMacro(
    "REQUEST_INFORMATION pass has been deprecated and no longer used");
  return false;
}
#endif

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::SetBlockVisibility(unsigned int index, bool visible)
{
  vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)->SetBlockVisibility(index, visible);
}

//----------------------------------------------------------------------------
bool vtkGeometryRepresentation::GetBlockVisibility(unsigned int index) const
{
  return vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)->GetBlockVisibility(index);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockVisibility(unsigned int index, bool)
{
  vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)->RemoveBlockVisibility(index);
}

//----------------------------------------------------------------------------
void vtkGeometryRepresentation::RemoveBlockVisibilities()
{
  vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)->RemoveBlockVisibilites();
}
