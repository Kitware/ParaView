/*=========================================================================

  Program:   ParaView
  Module:    vtkGlyph3DRepresentation.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGlyph3DRepresentation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkArrowSource.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper2.h"
#include "vtkDataObject.h"
#include "vtkDataObjectTree.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkGlyph3DMapper.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMPIMoveData.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPVCacheKeeper.h"
#include "vtkPVLODActor.h"
#include "vtkPVRenderView.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTransform.h"

namespace
{

//*****************************************************************************
// This is used to trick MoveData into accepting a polymorphic dataset. We
// don't know if the glyph source is going to be polydata or a
// vtkDataObjectTree, and MoveData requires that all ranks have the same
// dataset type. Since the client will be receiving the glyph source and doesn't
// know what type of dataset is on the server, we wrap the real dataset in a
// multiblock dataset.
class vtkGlyphRepresentationMultiBlockMaker : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkGlyphRepresentationMultiBlockMaker* New();
  vtkTypeMacro(vtkGlyphRepresentationMultiBlockMaker, vtkMultiBlockDataSetAlgorithm)

    protected : int RequestData(vtkInformation*, vtkInformationVector** inVec,
                  vtkInformationVector* outVec) override
  {
    vtkDataObject* inputDO = vtkDataObject::GetData(inVec[0], 0);
    vtkMultiBlockDataSet* outputMB = vtkMultiBlockDataSet::GetData(outVec, 0);

    vtkDataObject* clone = inputDO->NewInstance();
    clone->ShallowCopy(inputDO);
    outputMB->SetBlock(0, clone);
    clone->Delete();
    return 1;
  }

  int FillInputPortInformation(int, vtkInformation* info) override
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
    return 1;
  }
};
vtkStandardNewMacro(vtkGlyphRepresentationMultiBlockMaker)

} // end anon namespace

vtkStandardNewMacro(vtkGlyph3DRepresentation);
//----------------------------------------------------------------------------
vtkGlyph3DRepresentation::vtkGlyph3DRepresentation()
{
  this->SetNumberOfInputPorts(2);

  this->GlyphMultiBlockMaker = vtkGlyphRepresentationMultiBlockMaker::New();
  this->GlyphCacheKeeper = vtkPVCacheKeeper::New();

  this->GlyphCacheKeeper->SetInputConnection(this->GlyphMultiBlockMaker->GetOutputPort());

  this->GlyphMapper = vtkGlyph3DMapper::New();
  this->LODGlyphMapper = vtkGlyph3DMapper::New();
  this->GlyphActor = vtkPVLODActor::New();

  this->DummySource = vtkArrowSource::New();

  this->GlyphActor->SetMapper(this->GlyphMapper);
  this->GlyphActor->SetLODMapper(this->LODGlyphMapper);
  this->GlyphActor->SetProperty(this->Property);

  this->MeshVisibility = true;
  this->SetMeshVisibility(false);

  this->GlyphMapper->SetInterpolateScalarsBeforeMapping(0);
  this->LODGlyphMapper->SetInterpolateScalarsBeforeMapping(0);

  vtkCompositeDataDisplayAttributes* compositeAttributes =
    vtkCompositePolyDataMapper2::SafeDownCast(this->Mapper)->GetCompositeDataDisplayAttributes();
  this->GlyphMapper->SetBlockAttributes(compositeAttributes);
  this->LODGlyphMapper->SetBlockAttributes(compositeAttributes);
}

//----------------------------------------------------------------------------
vtkGlyph3DRepresentation::~vtkGlyph3DRepresentation()
{
  this->GlyphMultiBlockMaker->Delete();
  this->GlyphCacheKeeper->Delete();
  this->GlyphMapper->Delete();
  this->LODGlyphMapper->Delete();
  this->GlyphActor->Delete();
  this->DummySource->Delete();
}

//----------------------------------------------------------------------------
bool vtkGlyph3DRepresentation::AddToView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->AddActor(this->GlyphActor);
  }
  return this->Superclass::AddToView(view);
}

//----------------------------------------------------------------------------
bool vtkGlyph3DRepresentation::RemoveFromView(vtkView* view)
{
  vtkPVRenderView* rview = vtkPVRenderView::SafeDownCast(view);
  if (rview)
  {
    rview->GetRenderer()->RemoveActor(this->GlyphActor);
  }
  return this->Superclass::RemoveFromView(view);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetVisibility(bool val)
{
  this->Superclass::SetVisibility(val);
  this->GlyphActor->SetVisibility(val ? 1 : 0);
  this->Actor->SetVisibility((val && this->MeshVisibility) ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMeshVisibility(bool val)
{
  this->MeshVisibility = val;
  this->Actor->SetVisibility((val && this->MeshVisibility) ? 1 : 0);
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    return this->Superclass::FillInputPortInformation(port, info);
  }
  else if (port == 1)
  {
    info->Set(vtkAlgorithm::INPUT_IS_OPTIONAL(), 1);
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObjectTree");
    info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    return 1;
  }

  return 0;
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkAlgorithmOutput* glyphSourcePort = (inputVector[1]->GetNumberOfInformationObjects() == 1)
    ? this->GetInternalOutputPort(1)
    : this->DummySource->GetOutputPort();

  this->GlyphMultiBlockMaker->SetInputConnection(glyphSourcePort);

  // This is needed as for some reason when the input connection is changed,
  // in client-server mode, the GlyphMultiBlockMaker decides not to reexecute on
  // the client. That's throws of all logic to determine if data needs to be
  // delivered etc.
  this->GlyphMultiBlockMaker->Modified();

  this->GlyphCacheKeeper->SetCachingEnabled(this->GetUseCache());
  this->GlyphCacheKeeper->SetCacheTime(this->GetCacheKey());
  this->GlyphCacheKeeper->Update();

  return this->Superclass::RequestData(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::RequestUpdateExtent(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Superclass::RequestUpdateExtent(request, inputVector, outputVector))
  {
    return 0;
  }

  // For the input geometry used as the Glyph, we need it duplicated on all
  // ranks.
  for (int kk = 0; kk < inputVector[1]->GetNumberOfInformationObjects(); kk++)
  {
    vtkInformation* info = inputVector[1]->GetInformationObject(kk);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), 0);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), 1);
    info->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), 0);
    info->Set(vtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);
  }
  return 1;
}

//----------------------------------------------------------------------------
int vtkGlyph3DRepresentation::ProcessViewRequest(
  vtkInformationRequestKey* request_type, vtkInformation* inInfo, vtkInformation* outInfo)
{
  if (!this->Superclass::ProcessViewRequest(request_type, inInfo, outInfo))
  {
    return 0;
  }

  if (request_type == vtkPVView::REQUEST_UPDATE())
  {
    // A little trick to report correct bounds.
    // Representations are required to report the bounds for the rendered geometry
    // in vtkPVView::REQUEST_UPDATE() pass. For glyphs, the vtkGlyph3DMapper has
    // complicated logic to determine the bounds and we want to use that since
    // it respects orientation array, scale array and other options set on the
    // mapper. Now the complication is that glyph mapper on the "rendering
    // ranks" may not have the input set up yet (since it needs to be
    // redistributed by the view after this pass. So we do this, we change the
    // GlyphMapper's input be same as the data we have locally and then request
    // the GlyphMapper to compute bounds. The glyph mapper's input gets set back
    // to the delivered geometry in vtkPVView::REQUEST_RENDER() on the rendering
    // nodes, as is correct.
    double bounds[6];
    this->ComputeGlyphBounds(bounds);

    vtkNew<vtkMatrix4x4> matrix;
    this->GlyphActor->GetMatrix(matrix.GetPointer());
    vtkPVRenderView::SetGeometryBounds(inInfo, bounds, matrix.GetPointer());
    vtkPVRenderView::SetPiece(inInfo, this, this->GlyphCacheKeeper->GetOutput(), 0, 1);
  }
  else if (request_type == vtkPVView::REQUEST_UPDATE_LOD())
  {
    vtkPVRenderView::SetPieceLOD(inInfo, this, this->GlyphCacheKeeper->GetOutput(), 1);
  }

  if (request_type == vtkPVView::REQUEST_RENDER())
  {
    vtkAlgorithmOutput* producerPort = vtkPVRenderView::GetPieceProducer(inInfo, this, 0);
    vtkAlgorithmOutput* producerPortLOD = vtkPVRenderView::GetPieceProducerLOD(inInfo, this, 0);
    vtkAlgorithmOutput* producerGlyphPort = vtkPVRenderView::GetPieceProducer(inInfo, this, 1);
    vtkAlgorithmOutput* producerGlyphPortLOD =
      vtkPVRenderView::GetPieceProducerLOD(inInfo, this, 1);

    this->GlyphMapper->SetInputConnection(0, producerPort);
    this->LODGlyphMapper->SetInputConnection(0, producerPortLOD);

    // Extract the real glyph source from the MBDS wrapper (see note above
    // vtkGlyphRepresentationMultiBlockMaker)
    producerGlyphPort->GetProducer()->Update();
    vtkMultiBlockDataSet* glyphMBDS = vtkMultiBlockDataSet::SafeDownCast(
      producerGlyphPort->GetProducer()->GetOutputDataObject(producerGlyphPort->GetIndex()));
    if (glyphMBDS && glyphMBDS->GetNumberOfBlocks() == 1)
    {
      vtkDataObject* realGlyph = glyphMBDS->GetBlock(0);
      this->GlyphMapper->SetInputDataObject(1, realGlyph);
    }
    else
    {
      this->GlyphMapper->SetInputDataObject(1, NULL);
    }

    producerGlyphPortLOD->GetProducer()->Update();
    vtkMultiBlockDataSet* glyphMBDSLOD = vtkMultiBlockDataSet::SafeDownCast(
      producerGlyphPortLOD->GetProducer()->GetOutputDataObject(producerGlyphPortLOD->GetIndex()));
    if (glyphMBDSLOD && glyphMBDSLOD->GetNumberOfBlocks() == 1)
    {
      vtkDataObject* realGlyphLOD = glyphMBDSLOD->GetBlock(0);
      this->LODGlyphMapper->SetInputDataObject(1, realGlyphLOD);
    }
    else
    {
      this->LODGlyphMapper->SetInputDataObject(1, NULL);
    }

    bool lod = this->SuppressLOD ? false : (inInfo->Has(vtkPVRenderView::USE_LOD()) == 1);
    this->GlyphActor->SetEnableLOD(lod ? 1 : 0);

    // Figure out the range for source indexing, if enabled:
    if (this->GlyphMapper->GetSourceIndexing())
    {
      int tableSize = 0;
      if (this->GlyphMapper->GetUseSourceTableTree())
      {
        vtkDataObjectTree* sTT = this->GlyphMapper->GetSourceTableTree();
        if (sTT)
        {
          // Count the top level nodes:
          vtkDataObjectTreeIterator* iter = sTT->NewTreeIterator();
          iter->SetSkipEmptyNodes(false);
          iter->SetVisitOnlyLeaves(false);
          iter->SetTraverseSubTree(false);
          iter->InitTraversal();
          while (!iter->IsDoneWithTraversal())
          {
            ++tableSize;
            iter->GoToNextItem();
          }
          iter->Delete();
        } // end sTT exists
      }   // end using sTT
      else
      {
        tableSize = this->GlyphMapper->GetNumberOfInputConnections(1);
      }

      this->GlyphMapper->SetRange(0., static_cast<double>(std::max(0, tableSize - 1)));
      this->LODGlyphMapper->SetRange(0., static_cast<double>(std::max(0, tableSize - 1)));
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::MarkModified()
{
  if (!this->GetUseCache())
  {
    this->GlyphCacheKeeper->RemoveAllCaches();
  }
  this->Superclass::MarkModified();
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::UpdateColoringParameters()
{
  this->Superclass::UpdateColoringParameters();

  if (this->Mapper->GetScalarVisibility() == 0 ||
    this->Mapper->GetScalarMode() != VTK_SCALAR_MODE_USE_POINT_FIELD_DATA)
  {
    // we are not coloring the glyphs with scalars.
    const char* null = NULL;
    this->GlyphMapper->SetScalarVisibility(0);
    this->LODGlyphMapper->SetScalarVisibility(0);
    this->GlyphMapper->SelectColorArray(null);
    this->LODGlyphMapper->SelectColorArray(null);
    return;
  }

  this->GlyphMapper->SetScalarVisibility(1);
  this->GlyphMapper->SelectColorArray(this->Mapper->GetArrayName());
  this->GlyphMapper->SetUseLookupTableScalarRange(1);
  this->GlyphMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);

  this->LODGlyphMapper->SetScalarVisibility(1);
  this->LODGlyphMapper->SelectColorArray(this->Mapper->GetArrayName());
  this->LODGlyphMapper->SetUseLookupTableScalarRange(1);
  this->LODGlyphMapper->SetScalarMode(VTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::ComputeGlyphBounds(double bounds[6])
{
  // we're changing the glyph mapper's input connection temporarily.
  vtkSmartPointer<vtkAlgorithmOutput> port;
  if (this->GlyphMapper->GetNumberOfInputConnections(0) == 1)
  {
    port = this->GlyphMapper->GetInputConnection(0, 0);
  }

  this->GlyphMapper->SetInputConnection(this->CacheKeeper->GetOutputPort());
  this->GlyphMapper->GetBounds(bounds);
  this->GlyphMapper->SetInputConnection(port);
}

//----------------------------------------------------------------------------
bool vtkGlyph3DRepresentation::IsCached(double cache_key)
{
  return this->GlyphCacheKeeper->IsCached(cache_key) && this->Superclass::IsCached(cache_key);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//**************************************************************************
// Overridden to forward to vtkGlyph3DMapper
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetLookupTable(vtkScalarsToColors* val)
{
  this->GlyphMapper->SetLookupTable(val);
  this->LODGlyphMapper->SetLookupTable(val);
  this->Superclass::SetLookupTable(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMapScalars(int val)
{
  this->GlyphMapper->SetColorMode(val);
  this->LODGlyphMapper->SetColorMode(val);
  this->Superclass::SetMapScalars(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetInterpolateScalarsBeforeMapping(int val)
{
  // The GlyphMapper does not support InterpolateScalarsBeforeMapping==1. So
  // leave it at 0.
  this->Superclass::SetInterpolateScalarsBeforeMapping(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetStatic(int val)
{
  this->GlyphMapper->SetStatic(val);
  this->LODGlyphMapper->SetStatic(val);
  this->Superclass::SetStatic(val);
}

//**************************************************************************
// Forwarded to vtkGlyph3DMapper
//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMaskArray(const char* val)
{
  this->GlyphMapper->SetMaskArray(val);
  this->LODGlyphMapper->SetMaskArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaleArray(const char* val)
{
  this->GlyphMapper->SetScaleArray(val);
  this->LODGlyphMapper->SetScaleArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrientationArray(const char* val)
{
  this->GlyphMapper->SetOrientationArray(val);
  this->LODGlyphMapper->SetOrientationArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetSourceIndexArray(const char* val)
{
  this->GlyphMapper->SetSourceIndexArray(val);
  this->LODGlyphMapper->SetSourceIndexArray(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaling(bool val)
{
  this->GlyphMapper->SetScaling(val);
  this->LODGlyphMapper->SetScaling(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaleMode(int val)
{
  this->GlyphMapper->SetScaleMode(val);
  this->LODGlyphMapper->SetScaleMode(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScaleFactor(double val)
{
  this->GlyphMapper->SetScaleFactor(val);
  this->LODGlyphMapper->SetScaleFactor(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrient(bool val)
{
  this->GlyphMapper->SetOrient(val);
  this->LODGlyphMapper->SetOrient(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrientationMode(int val)
{
  this->GlyphMapper->SetOrientationMode(val);
  this->LODGlyphMapper->SetOrientationMode(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetMasking(bool val)
{
  this->GlyphMapper->SetMasking(val);
  this->LODGlyphMapper->SetMasking(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetSourceIndexing(bool val)
{
  this->GlyphMapper->SetSourceIndexing(val);
  this->LODGlyphMapper->SetSourceIndexing(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetUseSourceTableTree(bool val)
{
  this->GlyphMapper->SetUseSourceTableTree(val);
  this->LODGlyphMapper->SetUseSourceTableTree(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetUseCullingAndLOD(bool val)
{
  this->GlyphMapper->SetCullingAndLOD(val);
  this->LODGlyphMapper->SetCullingAndLOD(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetNumberOfLOD(int val)
{
  this->GlyphMapper->SetNumberOfLOD(val);
  this->LODGlyphMapper->SetNumberOfLOD(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetLODDistanceAndTargetReduction(int index, float dist, float reduc)
{
  this->GlyphMapper->SetLODDistanceAndTargetReduction(index, dist, reduc);
  this->LODGlyphMapper->SetLODDistanceAndTargetReduction(index, dist, reduc);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetColorByLODIndex(bool val)
{
  this->GlyphMapper->SetLODColoring(val);
  this->LODGlyphMapper->SetLODColoring(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrientation(double x, double y, double z)
{
  this->GlyphActor->SetOrientation(x, y, z);
  this->Superclass::SetOrientation(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetOrigin(double x, double y, double z)
{
  this->GlyphActor->SetOrigin(x, y, z);
  this->Superclass::SetOrigin(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetPickable(int val)
{
  this->GlyphActor->SetPickable(val);
  this->Superclass::SetPickable(val);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetPosition(double x, double y, double z)
{
  this->GlyphActor->SetPosition(x, y, z);
  this->Superclass::SetPosition(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetScale(double x, double y, double z)
{
  this->GlyphActor->SetScale(x, y, z);
  this->Superclass::SetScale(x, y, z);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetTexture(vtkTexture* tex)
{
  // don't think it makes sense to add textures to glyphs.
  this->Superclass::SetTexture(tex);
}

//----------------------------------------------------------------------------
void vtkGlyph3DRepresentation::SetUserTransform(const double matrix[16])
{
  vtkNew<vtkTransform> transform;
  transform->SetMatrix(matrix);
  this->GlyphActor->SetUserTransform(transform.GetPointer());
  this->Superclass::SetUserTransform(matrix);
}
