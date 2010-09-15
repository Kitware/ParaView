/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGeometryFilter.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVGeometryFilter.h"

#include "vtkAlgorithmOutput.h"
#include "vtkAppendPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellTypes.h"
#include "vtkCleanArrays.h"
#include "vtkCommand.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkGarbageCollector.h"
#include "vtkGenericDataSet.h"
#include "vtkGenericGeometryFilter.h"
#include "vtkGeometryFilter.h"
#include "vtkHierarchicalBoxDataIterator.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkHyperOctree.h"
#include "vtkHyperOctreeSurfaceFilter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkOutlineSource.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolygon.h"
#include "vtkPVRecoverGeometryWireframe.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridOutlineFilter.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStripper.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridOutlineFilter.h"
#include "vtkTimerLog.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnstructuredGridGeometryFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkMultiPieceDataSet.h"

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <assert.h>

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkPVGeometryFilter);
vtkCxxSetObjectMacro(vtkPVGeometryFilter, Controller, vtkMultiProcessController);
vtkInformationKeyMacro(vtkPVGeometryFilter, POINT_OFFSETS, IntegerVector);
vtkInformationKeyMacro(vtkPVGeometryFilter, VERTS_OFFSETS, IntegerVector);
vtkInformationKeyMacro(vtkPVGeometryFilter, LINES_OFFSETS, IntegerVector);
vtkInformationKeyMacro(vtkPVGeometryFilter, POLYS_OFFSETS, IntegerVector);
vtkInformationKeyMacro(vtkPVGeometryFilter, STRIPS_OFFSETS, IntegerVector);
class vtkPVGeometryFilter::BoundsReductionOperation : public vtkCommunicator::Operation
{
public:
  // Subclasses must overload this method, which performs the actual
  // operations.  The methods should first do a reintepret cast of the arrays
  // to the type suggestsed by \c datatype (which will be one of the VTK type
  // identifiers like VTK_INT, etc.).  Both arrays are considered top be
  // length entries.  The method should perform the operation A*B (where * is
  // a placeholder for whatever operation is actually performed) and store the
  // result in B.  The operation is assumed to be associative.  Commutativity
  // is specified by the Commutative method.
  virtual void Function(const void *A, void *B, vtkIdType length,
    int datatype)
    {
    assert((datatype == VTK_DOUBLE) && (length==6));
    (void)datatype;
    (void)length;
    const double* bdsA = reinterpret_cast<const double*>(A);
    double* bdsB = reinterpret_cast<double*>(B);
    if (bdsA[0] < bdsB[0])
      {
      bdsB[0] = bdsA[0];
      }
    if (bdsA[1] > bdsB[1])
      {
      bdsB[1] = bdsA[1];
      }
    if (bdsA[2] < bdsB[2])
      {
      bdsB[2] = bdsA[2];
      }
    if (bdsA[3] > bdsB[3])
      {
      bdsB[3] = bdsA[3];
      }
    if (bdsA[4] < bdsB[4])
      {
      bdsB[4] = bdsA[4];
      }
    if (bdsA[5] > bdsB[5])
      {
      bdsB[5] = bdsA[5];
      }
    }

  // Description:
  // Subclasses override this method to specify whether their operation
  // is commutative.  It should return 1 if commutative or 0 if not.
  virtual int Commutative()
    {
    return 1;
    }
};

//----------------------------------------------------------------------------
vtkPVGeometryFilter::vtkPVGeometryFilter ()
{
  this->OutlineFlag = 0;
  this->UseOutline = 1;
  this->UseStrips = 0;
  this->GenerateCellNormals = 1;
  this->NonlinearSubdivisionLevel = 1;

  this->DataSetSurfaceFilter = vtkDataSetSurfaceFilter::New();
  this->GenericGeometryFilter=vtkGenericGeometryFilter::New();
  this->UnstructuredGridGeometryFilter=vtkUnstructuredGridGeometryFilter::New();
  this->RecoverWireframeFilter = vtkPVRecoverGeometryWireframe::New();

  // Setup a callback for the internal readers to report progress.
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(
    &vtkPVGeometryFilter::InternalProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->OutlineSource = vtkOutlineSource::New();

  this->PassThroughCellIds = 1;
  this->PassThroughPointIds = 1;
  this->ForceUseStrips = 0;
  this->StripModFirstPass = 1;
  this->MakeOutlineOfInput = 0;

  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_RANGES(), 1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_BOUNDS(), 1);
  this->GetInformation()->Set(vtkAlgorithm::PRESERVES_TOPOLOGY(), 1);
}

//----------------------------------------------------------------------------
vtkPVGeometryFilter::~vtkPVGeometryFilter ()
{
  // Be careful how you delete these so that you don't foul up the garbage
  // collector.
  if (this->DataSetSurfaceFilter)
    {
    vtkDataSetSurfaceFilter *tmp = this->DataSetSurfaceFilter;
    this->DataSetSurfaceFilter = NULL;
    tmp->Delete();
    }
  if (this->GenericGeometryFilter)
    {
    vtkGenericGeometryFilter *tmp = this->GenericGeometryFilter;
    this->GenericGeometryFilter = NULL;
    tmp->Delete();
    }
  if (this->UnstructuredGridGeometryFilter)
    {
    vtkUnstructuredGridGeometryFilter *tmp=this->UnstructuredGridGeometryFilter;
    this->UnstructuredGridGeometryFilter = NULL;
    tmp->Delete();
    }
  if (this->RecoverWireframeFilter)
    {
    vtkPVRecoverGeometryWireframe *tmp = this->RecoverWireframeFilter;
    this->RecoverWireframeFilter = NULL;
    tmp->Delete();
    }
  this->OutlineSource->Delete();
  this->InternalProgressObserver->Delete();
  this->SetController(0);
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestDataObject(vtkInformation*,
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject *input = vtkDataObject::GetData(inputVector[0], 0);
  vtkDataObject *output = vtkDataSet::GetData(outputVector, 0);

  if (input)
    {
    // If input is composite-data, then output is multi-block of polydata,
    // otherwise it's a poly data.
    if (vtkCompositeDataSet::SafeDownCast(input))
      {
      if (vtkMultiBlockDataSet::SafeDownCast(output) == NULL)
        {
        if (vtkMultiBlockDataSet::SafeDownCast(input))
          {
          // Some developers have sub-classed vtkMultiBlockDataSet, in which
          // case, we try to preserve the type.
          output = input->NewInstance();
          }
        else
          {
          output = vtkMultiBlockDataSet::New();
          }
        output->SetPipelineInformation(outputVector->GetInformationObject(0));
        output->FastDelete();
        }
      return 1;
      }

    if (vtkPolyData::SafeDownCast(output) == NULL)
      {
      output = vtkPolyData::New();
      output->SetPipelineInformation(outputVector->GetInformationObject(0));
      output->FastDelete();
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
vtkExecutive* vtkPVGeometryFilter::CreateDefaultExecutive()
{
  return vtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::InternalProgressCallbackFunction(vtkObject *arg,
                                                           unsigned long,
                                                           void* clientdata,
                                                           void*)
{
  reinterpret_cast<vtkPVGeometryFilter*>(clientdata)
    ->InternalProgressCallback(static_cast<vtkAlgorithm *>(arg));
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::InternalProgressCallback(vtkAlgorithm *algorithm)
{
  // This limits progress for only the DataSetSurfaceFilter.
  float progress = algorithm->GetProgress();
  if (progress > 0 && progress < 1)
    {
    this->UpdateProgress(progress);
    }
  if (this->AbortExecute)
    {
    algorithm->SetAbortExecute(1);
    }
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::CheckAttributes(vtkDataObject* input)
{
  if (input->IsA("vtkDataSet"))
    {
    if (static_cast<vtkDataSet*>(input)->CheckAttributes())
      {
      return 1;
      }
    }
  else if (input->IsA("vtkCompositeDataSet"))
    {
    vtkCompositeDataSet* compInput =
      static_cast<vtkCompositeDataSet*>(input);
    vtkCompositeDataIterator* iter = compInput->NewIterator();
    iter->GoToFirstItem();
    while (!iter->IsDoneWithTraversal())
      {
      vtkDataObject* curDataSet = iter->GetCurrentDataObject();
      if (curDataSet && this->CheckAttributes(curDataSet))
        {
        return 1;
        }
      iter->GoToNextItem();
      }
    iter->Delete();
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestInformation(
  vtkInformation*, vtkInformationVector** vtkNotUsed(inVectors), vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // RequestData() synchronizes (communicates among processes), so we need
  // all procs to call RequestData().
  outInfo->Set(vtkStreamingDemandDrivenPipeline::MAXIMUM_NUMBER_OF_PIECES(), -1);

  return 1;
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestUpdateExtent(vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  /*
  vtkUnstructuredGrid* ug_input =
    vtkUnstructuredGrid::GetData(inputVector[0], 0);
  vtkCompositeDataSet* cd_input =
    vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (ug_input || cd_input)
    {
    vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation *outInfo = outputVector->GetInformationObject(0);
    int numPieces, ghostLevels;
    numPieces =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    ghostLevels =
      outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    if (numPieces > 1)
      {
      ++ghostLevels;
      }
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      ghostLevels);
    }

  */
  return this->Superclass::RequestUpdateExtent(request, inputVector,
    outputVector);
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ExecuteBlock(
  vtkDataObject* input, vtkPolyData* output, int doCommunicate)
{
  if (this->UseOutline && this->MakeOutlineOfInput)
    {
    vtkAlgorithmOutput *pport = input->GetProducerPort();
    vtkDataObject *insin = NULL;
    if (pport)
      {
      vtkAlgorithm *alg = pport->GetProducer();
      if (alg &&
          alg->GetNumberOfInputPorts() &&
          alg->GetNumberOfInputConnections(0))
        {
        insin = alg->GetInputDataObject(0,0);
        }
      }
    if (insin)
      {
      input = insin;
      }
    }

  if (input->IsA("vtkImageData"))
    {
    this->ImageDataExecute(static_cast<vtkImageData*>(input), output, doCommunicate);
    return;
    }

  if (input->IsA("vtkStructuredGrid"))
    {
    this->StructuredGridExecute(static_cast<vtkStructuredGrid*>(input), output);
    return;
    }

  if (input->IsA("vtkRectilinearGrid"))
    {
    this->RectilinearGridExecute(static_cast<vtkRectilinearGrid*>(input),output);
    return;
    }

  if (input->IsA("vtkUnstructuredGrid"))
    {
    this->UnstructuredGridExecute(
      static_cast<vtkUnstructuredGrid*>(input), output, doCommunicate);
    return;
    }

  if (input->IsA("vtkPolyData"))
    {
    this->PolyDataExecute(static_cast<vtkPolyData*>(input), output, doCommunicate);
    return;
    }
  if (input->IsA("vtkHyperOctree"))
    {
    this->OctreeExecute(static_cast<vtkHyperOctree*>(input), output, doCommunicate);
    return;
    }
  if (input->IsA("vtkDataSet"))
    {
    this->DataSetExecute(static_cast<vtkDataSet*>(input), output, doCommunicate);
    return;
    }
  if (input->IsA("vtkGenericDataSet"))
    {
    this->GenericDataSetExecute(static_cast<vtkGenericDataSet*>(input), output, doCommunicate);
    return;
    }
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestData(vtkInformation* request,
                                     vtkInformationVector** inputVector,
                                     vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0], 0);
  if (vtkCompositeDataSet::SafeDownCast(input))
    {
    vtkGarbageCollector::DeferredCollectionPush();
    vtkTimerLog::MarkStartEvent("vtkPVGeometryFilter::RequestData");
    this->RequestCompositeData(request, inputVector, outputVector);
    vtkTimerLog::MarkEndEvent("vtkPVGeometryFilter::RequestData");

    vtkTimerLog::MarkStartEvent("vtkPVGeometryFilter::GarbageCollect");
    vtkGarbageCollector::DeferredCollectionPop();
    vtkTimerLog::MarkEndEvent("vtkPVGeometryFilter::GarbageCollect");
    return 1;
    }

  vtkPolyData *output = vtkPolyData::GetData(outputVector, 0);
  assert(output != NULL);

  this->ExecuteBlock(input, output, 1);
  this->ExecuteCellNormals(output, 1);
  this->RemoveGhostCells(output);
  return 1;
}

namespace
{
  static void vtkPVGeometryFilterMergePieces(vtkMultiPieceDataSet* mp)
    {
    unsigned int num_pieces = mp->GetNumberOfPieces();
    if (num_pieces==0)
      {
      return;
      }

    vtkstd::vector<vtkPolyData*> inputs;
    vtkstd::vector<int> points_counts, cell_counts, verts_counts, polys_counts,
      lines_counts, strips_counts;

    polys_counts.resize(num_pieces); verts_counts.resize(num_pieces);
    lines_counts.resize(num_pieces); strips_counts.resize(num_pieces);
    points_counts.resize(num_pieces); cell_counts.resize(num_pieces);
    for (unsigned int cc=0; cc < num_pieces; cc++)
      {
      vtkPolyData* piece = vtkPolyData::SafeDownCast(mp->GetPiece(cc));
      if (piece && piece->GetNumberOfPoints() > 0)
        {
        inputs.push_back(piece);
        points_counts[cc] = piece->GetNumberOfPoints();
        cell_counts[cc] = piece->GetNumberOfCells();
        verts_counts[cc] = piece->GetNumberOfVerts();
        polys_counts[cc] = piece->GetNumberOfPolys();
        lines_counts[cc] = piece->GetNumberOfLines();
        strips_counts[cc] = piece->GetNumberOfStrips();
        }
      }

    if (inputs.size() == 0)
      {
      // not much to do, this is an empty multi-piece.
      return;
      }

    vtkPolyData* output = vtkPolyData::New();
    vtkAppendPolyData* appender = vtkAppendPolyData::New();
    appender->ExecuteAppend(output, &inputs[0],
      static_cast<int>(inputs.size()));
    appender->Delete();
    inputs.clear();

    vtkstd::vector<int> points_offsets, verts_offsets, lines_offsets,
      polys_offsets, strips_offsets;
    polys_offsets.resize(num_pieces); verts_offsets.resize(num_pieces);
    lines_offsets.resize(num_pieces); strips_offsets.resize(num_pieces);
    points_offsets.resize(num_pieces);
    points_offsets[0] = 0;
    verts_offsets[0] = 0;
    lines_offsets[0] = output->GetNumberOfVerts();
    polys_offsets[0] = lines_offsets[0] + output->GetNumberOfLines();
    strips_offsets[0] = polys_offsets[0] + output->GetNumberOfPolys();
    for (unsigned int cc=1; cc < num_pieces; cc++)
      {
      points_offsets[cc] = points_offsets[cc-1] + points_counts[cc-1];
      verts_offsets[cc] = verts_offsets[cc-1] + verts_counts[cc-1];
      lines_offsets[cc] = lines_offsets[cc-1] + lines_counts[cc-1];
      polys_offsets[cc] = polys_offsets[cc-1] + polys_counts[cc-1];
      strips_offsets[cc] = strips_offsets[cc-1] + strips_counts[cc-1];
      }

    for (unsigned int cc=0; cc < num_pieces; cc++)
      {
      mp->SetPiece(cc, NULL);
      }

    mp->SetPiece(0, output);
    output->FastDelete();

    vtkInformation* metadata = mp->GetMetaData(static_cast<unsigned int>(0));
    metadata->Set(vtkPVGeometryFilter::POINT_OFFSETS(),
      &points_offsets[0], static_cast<int>(num_pieces));
    metadata->Set(vtkPVGeometryFilter::VERTS_OFFSETS(),
      &verts_offsets[0], static_cast<int>(num_pieces));
    metadata->Set(vtkPVGeometryFilter::LINES_OFFSETS(),
      &lines_offsets[0], static_cast<int>(num_pieces));
    metadata->Set(vtkPVGeometryFilter::POLYS_OFFSETS(),
      &polys_offsets[0], static_cast<int>(num_pieces));
    metadata->Set(vtkPVGeometryFilter::STRIPS_OFFSETS(),
      &strips_offsets[0], static_cast<int>(num_pieces));
    }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void vtkPVGeometryFilter::AddCompositeIndex(vtkPolyData* pd, unsigned int index)
{
  vtkUnsignedIntArray* cindex = vtkUnsignedIntArray::New();
  cindex->SetNumberOfComponents(1);
  cindex->SetNumberOfTuples(pd->GetNumberOfCells());
  cindex->FillComponent(0, index);
  cindex->SetName("vtkCompositeIndex");
  pd->GetCellData()->AddArray(cindex);
  cindex->FastDelete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::AddHierarchicalIndex(vtkPolyData* pd,
  unsigned int level, unsigned int index)
{
  vtkUnsignedIntArray* dslevel = vtkUnsignedIntArray::New();
  dslevel->SetNumberOfTuples(pd->GetNumberOfCells());
  dslevel->FillComponent(0, level);
  dslevel->SetName("vtkAMRLevel");
  pd->GetCellData()->AddArray(dslevel);
  dslevel->FastDelete();

  vtkUnsignedIntArray* dsindex = vtkUnsignedIntArray::New();
  dsindex->SetNumberOfTuples(pd->GetNumberOfCells());
  dsindex->FillComponent(0, index);
  dsindex->SetName("vtkAMRIndex");
  pd->GetCellData()->AddArray(dsindex);
  dsindex->FastDelete();
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::RequestCompositeData(vtkInformation*,
                                              vtkInformationVector** inputVector,
                                              vtkInformationVector* outputVector)
{
  vtkTimerLog::MarkStartEvent("vtkPVGeometryFilter::RequestCompositeData");

  vtkCompositeDataSet *output = vtkCompositeDataSet::GetData(outputVector, 0);
  if (!output)
    {
    return 0;
    }

  vtkCompositeDataSet *input = vtkCompositeDataSet::GetData(inputVector[0], 0);
  if (!input)
    {
    return 0;
    }
  output->CopyStructure(input);

  vtkTimerLog::MarkStartEvent("vtkPVGeometryFilter::CheckAttributes");
  if (this->CheckAttributes(input))
    {
    return 0;
    }
  vtkTimerLog::MarkEndEvent("vtkPVGeometryFilter::CheckAttributes");

  vtkTimerLog::MarkStartEvent("vtkPVGeometryFilter::ExecuteCompositeDataSet");
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(input->NewIterator());

  vtkHierarchicalBoxDataIterator* hdIter =
    vtkHierarchicalBoxDataIterator::SafeDownCast(iter);
  unsigned int totNumBlocks=0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    // iter skips empty blocks automatically.
    totNumBlocks++;
    }

  vtkstd::vector<unsigned char> non_null_leaves;
  non_null_leaves.reserve(totNumBlocks); //just an estimate.

  int numInputs = 0;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataObject* block = iter->GetCurrentDataObject();

    vtkPolyData* tmpOut = vtkPolyData::New();
    this->ExecuteBlock(block, tmpOut, 0);
    this->ExecuteCellNormals(tmpOut, 0);
    this->RemoveGhostCells(tmpOut);
    //skip empty nodes.
    if (tmpOut->GetNumberOfPoints() > 0)
      {
      unsigned int current_flat_index = iter->GetCurrentFlatIndex();
      non_null_leaves.resize(current_flat_index+1);
      non_null_leaves[current_flat_index] = 1;
      output->SetDataSet(iter, tmpOut);
      tmpOut->FastDelete();

      this->AddCompositeIndex(tmpOut, current_flat_index);
      }
    else
      {
      tmpOut->Delete();
      tmpOut = NULL;
      }

    if (hdIter)
      {
      // This will be used later by vtkSelectionConverter to realize that this
      // multi-block of polydatas is actually coming from an AMR.
      vtkInformation* metadata = output->GetMetaData(iter);
      metadata->Set(vtkSelectionNode::HIERARCHICAL_LEVEL(),
        hdIter->GetCurrentLevel());
      metadata->Set(vtkSelectionNode::HIERARCHICAL_INDEX(),
        hdIter->GetCurrentIndex());
      if (tmpOut)
        {
        this->AddHierarchicalIndex(tmpOut,
          hdIter->GetCurrentLevel(), hdIter->GetCurrentIndex());
        }
      }

    numInputs++;
    this->UpdateProgress(static_cast<float>(numInputs)/totNumBlocks);
    }
  vtkTimerLog::MarkEndEvent("vtkPVGeometryFilter::ExecuteCompositeDataSet");

  // Merge mutli-pieces to avoid efficiency setbacks when ordered
  // compositing is employed.
  iter.TakeReference(output->NewIterator());
  iter->VisitOnlyLeavesOff();

  vtkstd::vector<vtkMultiPieceDataSet*> pieces_to_merge;
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
    iter->GoToNextItem())
    {
    vtkDataObject* curNode = iter->GetCurrentDataObject();
    if (curNode && vtkMultiPieceDataSet::SafeDownCast(curNode))
      {
      vtkMultiPieceDataSet* piece = vtkMultiPieceDataSet::SafeDownCast(curNode);
      pieces_to_merge.push_back(piece);
      }
    }
  for (size_t cc=0; cc < pieces_to_merge.size(); cc++)
    {
    vtkPVGeometryFilterMergePieces(pieces_to_merge[cc]);
    }

  // Now, when running in parallel, processes may have NULL-leaf nodes at
  // different locations. To make our life easier in subsquent filtering such as
  // vtkAllToNRedistributeCompositePolyData or vtkKdTreeManager we ensure that
  // all NULL-leafs match up across processes i.e. if any leaf is non-null on
  // any process, then all other processes add empty polydatas for that leaf.
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
    {
    int count = static_cast<int>(non_null_leaves.size());
    int reduced_size;
    this->Controller->AllReduce(&count, &reduced_size, 1, vtkCommunicator::MAX_OP);
    assert(reduced_size >= static_cast<int>(non_null_leaves.size()));
    non_null_leaves.resize(reduced_size, 0);
    // if reduced_size ==0, then all processes have no non-null-leaves, so
    // nothing special to do here.
    if (reduced_size != 0)
      {
      vtkstd::vector<unsigned char>reduced_non_null_leaves;
      reduced_non_null_leaves.resize(reduced_size, 0);
      this->Controller->AllReduce(
        &non_null_leaves[0], &reduced_non_null_leaves[0],
        reduced_size, vtkCommunicator::MAX_OP);

      vtkPolyData* trivalInput = vtkPolyData::New();
      iter->SkipEmptyNodesOff();
      iter->VisitOnlyLeavesOff();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal();
        iter->GoToNextItem())
        {
        unsigned int index = iter->GetCurrentFlatIndex();
        if (iter->GetCurrentDataObject() == NULL &&
          index < static_cast<unsigned int>(reduced_non_null_leaves.size()) &&
          reduced_non_null_leaves[index] != 0)
          {
          output->SetDataSet(iter, trivalInput);
          }
        }
      trivalInput->Delete();
      }
    }

  vtkTimerLog::MarkEndEvent("vtkPVGeometryFilter::RequestCompositeData");
  return 1;
}

//----------------------------------------------------------------------------
// We need to change the mapper.  Now it always flat shades when cell normals
// are available.
void vtkPVGeometryFilter::ExecuteCellNormals(vtkPolyData* output, int doCommunicate)
{
  if ( ! this->GenerateCellNormals)
    {
    return;
    }

  // Do not generate cell normals if any of the processes
  // have lines, verts or strips.
  vtkCellArray* aPrim;
  int skip = 0;
  aPrim = output->GetVerts();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    skip = 1;
    }
  aPrim = output->GetLines();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    skip = 1;
    }
  aPrim = output->GetStrips();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    skip = 1;
    }
  if( this->Controller && doCommunicate )
    {
    int reduced_skip = 0;
    if (!this->Controller->AllReduce(&skip, &reduced_skip, 1,
        vtkCommunicator::MAX_OP))
      {
      vtkErrorMacro("Failed to reduce correctly.");
      skip = 1;
      }
    else
      {
      skip = reduced_skip;
      }
    }
  if (skip)
    {
    return;
    }

  vtkIdType* endCellPtr;
  vtkIdType* cellPtr;
  vtkIdType *pts = 0;
  vtkIdType npts = 0;
  double polyNorm[3];
  vtkFloatArray* cellNormals = vtkFloatArray::New();
  cellNormals->SetName("cellNormals");
  cellNormals->SetNumberOfComponents(3);
  cellNormals->Allocate(3*output->GetNumberOfCells());

  aPrim = output->GetPolys();
  if (aPrim && aPrim->GetNumberOfCells())
    {
    vtkPoints* p = output->GetPoints();

    cellPtr = aPrim->GetPointer();
    endCellPtr = cellPtr+aPrim->GetNumberOfConnectivityEntries();

    while (cellPtr < endCellPtr)
      {
      npts = *cellPtr++;
      pts = cellPtr;
      cellPtr += npts;

      vtkPolygon::ComputeNormal(p,npts,pts,polyNorm);
      cellNormals->InsertNextTuple(polyNorm);
      }
    }

  if (cellNormals->GetNumberOfTuples() != output->GetNumberOfCells())
    {
    vtkErrorMacro("Number of cell normals does not match output.");
    cellNormals->Delete();
    return;
    }

  output->GetCellData()->AddArray(cellNormals);
  output->GetCellData()->SetActiveNormals(cellNormals->GetName());
  cellNormals->Delete();
  cellNormals = NULL;
}


//----------------------------------------------------------------------------
void vtkPVGeometryFilter::DataSetExecute(
  vtkDataSet* input, vtkPolyData* output, int doCommunicate)
{
  double bds[6];
  int procid = 0;
  int numProcs = 1;

  if (!doCommunicate && input->GetNumberOfPoints() == 0)
    {
    return;
    }

  if (this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  input->GetBounds(bds);

  vtkPVGeometryFilter::BoundsReductionOperation operation;
  if ( procid && doCommunicate )
    {
    // Satellite node
    this->Controller->Reduce(bds, NULL, 6, &operation, 0);
    }
  else
    {
    if (this->Controller && doCommunicate)
      {
      double tmp[6];
      this->Controller->Reduce(bds, tmp, 6, &operation, 0);
      memcpy(bds, tmp, 6*sizeof(double));
      }

    if (bds[1] >= bds[0] && bds[3] >= bds[2] && bds[5] >= bds[4])
      {
      // only output in process 0.
      this->OutlineSource->SetBounds(bds);
      this->OutlineSource->Update();

      output->SetPoints(this->OutlineSource->GetOutput()->GetPoints());
      output->SetLines(this->OutlineSource->GetOutput()->GetLines());
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::GenericDataSetExecute(
  vtkGenericDataSet* input, vtkPolyData* output, int doCommunicate)
{
  double bds[6];
  int procid = 0;
  int numProcs = 1;

  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;

    // Geometry filter
    this->GenericGeometryFilter->SetInput(input);

    // Observe the progress of the internal filter.
    this->GenericGeometryFilter->AddObserver(vtkCommand::ProgressEvent,
                                            this->InternalProgressObserver);
    this->GenericGeometryFilter->Update();
    // The internal filter is finished.  Remove the observer.
    this->GenericGeometryFilter->RemoveObserver(this->InternalProgressObserver);

    output->ShallowCopy(this->GenericGeometryFilter->GetOutput());

    return;
    }

  // Just outline
  this->OutlineFlag = 1;

  if (!doCommunicate && input->GetNumberOfPoints() == 0)
    {
    return;
    }

  if (this->Controller )
    {
    procid = this->Controller->GetLocalProcessId();
    numProcs = this->Controller->GetNumberOfProcesses();
    }

  input->GetBounds(bds);

  vtkPVGeometryFilter::BoundsReductionOperation operation;
  if ( procid && doCommunicate )
    {
    // Satellite node
    this->Controller->Reduce(bds, NULL, 6, &operation, 0);
    }
  else
    {
    if (doCommunicate)
      {
      double tmp[6];
      this->Controller->Reduce(bds, tmp, 6, &operation, 0);
      memcpy(bds, tmp, 6*sizeof(double));
      }

    // only output in process 0.
    this->OutlineSource->SetBounds(bds);
    this->OutlineSource->Update();

    output->SetPoints(this->OutlineSource->GetOutput()->GetPoints());
    output->SetLines(this->OutlineSource->GetOutput()->GetLines());
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::ImageDataExecute(vtkImageData *input,
                                           vtkPolyData* output,
                                           int doCommunicate)
{
  double *spacing;
  double *origin;
  int *ext;
  double bounds[6];

  // If doCommunicate is false, use extent because the block is
  // entirely contained in this process.
  if (doCommunicate)
    {
    ext = input->GetWholeExtent();
    }
  else
    {
    ext = input->GetExtent();
    }

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    if (input->GetNumberOfCells() > 0)
      {
      this->DataSetSurfaceFilter->StructuredExecute(input,
        output, input->GetExtent(), ext);
      }
    this->OutlineFlag = 0;
    return;
    }
  this->OutlineFlag = 1;

  //
  // Otherwise, let OutlineSource do all the work
  //

  if (ext[1] >= ext[0] && ext[3] >= ext[2] && ext[5] >= ext[4] &&
    (output->GetUpdatePiece() == 0 || !doCommunicate))
    {
    spacing = input->GetSpacing();
    origin = input->GetOrigin();

    bounds[0] = spacing[0] * ((float)ext[0]) + origin[0];
    bounds[1] = spacing[0] * ((float)ext[1]) + origin[0];
    bounds[2] = spacing[1] * ((float)ext[2]) + origin[1];
    bounds[3] = spacing[1] * ((float)ext[3]) + origin[1];
    bounds[4] = spacing[2] * ((float)ext[4]) + origin[2];
    bounds[5] = spacing[2] * ((float)ext[5]) + origin[2];

    vtkOutlineSource *outline = vtkOutlineSource::New();
    outline->SetBounds(bounds);
    outline->Update();

    output->SetPoints(outline->GetOutput()->GetPoints());
    output->SetLines(outline->GetOutput()->GetLines());
    output->SetPolys(outline->GetOutput()->GetPolys());
    outline->Delete();
    }
  else
    {
    vtkPoints* pts = vtkPoints::New();
    output->SetPoints(pts);
    pts->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::StructuredGridExecute(vtkStructuredGrid* input,
                                                vtkPolyData* output)
{
  int *ext;

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    if (input->GetNumberOfCells() > 0)
      {
      this->DataSetSurfaceFilter->StructuredExecute(input, output, input->GetExtent(),
        input->GetWholeExtent());
      }
    this->OutlineFlag = 0;
    return;
    }
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //


  vtkStructuredGridOutlineFilter *outline = vtkStructuredGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::RectilinearGridExecute(vtkRectilinearGrid* input,
                                                 vtkPolyData* output)
{
  int *ext;

  ext = input->GetWholeExtent();

  // If 2d then default to superclass behavior.
//  if (ext[0] == ext[1] || ext[2] == ext[3] || ext[4] == ext[5] ||
//      !this->UseOutline)
  if (!this->UseOutline)
    {
    if (input->GetNumberOfCells() > 0)
      {
      this->DataSetSurfaceFilter->StructuredExecute(input, output,
        input->GetExtent(), input->GetWholeExtent());
      }
    this->OutlineFlag = 0;
    return;
    }
  this->OutlineFlag = 1;

  //
  // Otherwise, let Outline do all the work
  //

  vtkRectilinearGridOutlineFilter *outline = vtkRectilinearGridOutlineFilter::New();
  // Because of streaming, it is important to set the input and not copy it.
  outline->SetInput(input);
  outline->GetOutput()->SetUpdateNumberOfPieces(output->GetUpdateNumberOfPieces());
  outline->GetOutput()->SetUpdatePiece(output->GetUpdatePiece());
  outline->GetOutput()->SetUpdateGhostLevel(output->GetUpdateGhostLevel());
  outline->GetOutput()->Update();

  output->CopyStructure(outline->GetOutput());
  outline->Delete();
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::UnstructuredGridExecute(
  vtkUnstructuredGrid* input, vtkPolyData* output, int doCommunicate)
{
  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;

    bool handleSubdivision = false;
    if (this->NonlinearSubdivisionLevel > 0)
      {
      // Check to see if the data actually has nonlinear cells.  Handling
      // nonlinear cells adds unnecessary work if we only have linear cells.
      vtkUnsignedCharArray *types = input->GetCellTypesArray();
      vtkIdType numCells = input->GetNumberOfCells();
      for (vtkIdType i = 0; i < numCells; i++)
        {
        if (!vtkCellTypes::IsLinear(types->GetValue(i)))
          {
          handleSubdivision = true;
          break;
          }
        }
      }

    vtkSmartPointer<vtkIdTypeArray> facePtIds2OriginalPtIds;

    if (handleSubdivision)
      {
      // Use the vtkUnstructuredGridGeometryFilter to extract 2D surface cells
      // from the geometry.  This is important to extract an appropriate
      // wireframe in vtkPVRecoverGeometryWireframe.  Also, at the time of this
      // writing vtkDataSetSurfaceFilter only properly subdivides 2D cells past
      // level 1.
      VTK_CREATE(vtkUnstructuredGrid, inputClone);
      inputClone->ShallowCopy(input);
      this->UnstructuredGridGeometryFilter->SetInput(inputClone);

      // Let the vtkUnstructuredGridGeometryFilter record from which point and
      // cell each face comes from in the standard vtkOriginalCellIds array.
      this->UnstructuredGridGeometryFilter->SetPassThroughCellIds(
                                                      this->PassThroughCellIds);
      this->UnstructuredGridGeometryFilter->SetPassThroughPointIds(
                                                     this->PassThroughPointIds);

      // Observe the progress of the internal filter.
      // TODO: Make the consecutive internal filter execution have monotonically
      // increasing progress rather than restarting for every internal filter.
      this->UnstructuredGridGeometryFilter->AddObserver(
                                                vtkCommand::ProgressEvent,
                                                this->InternalProgressObserver);
      this->UnstructuredGridGeometryFilter->Update();
      // The internal filter finished.  Remove the observer.
      this->UnstructuredGridGeometryFilter->RemoveObserver(
                                                this->InternalProgressObserver);

      this->UnstructuredGridGeometryFilter->SetInput(NULL);

      // Feed the extracted surface as the input to the rest of the processing.
      input->ShallowCopy(this->UnstructuredGridGeometryFilter->GetOutput());

      // Keep a handle to the vtkOriginalPointIds array.  We might need it.
      facePtIds2OriginalPtIds = vtkIdTypeArray::SafeDownCast(
                        input->GetPointData()->GetArray("vtkOriginalPointIds"));

      // Flag the data set surface filter to record original cell ids, but do it
      // in a specially named array that vtkPVRecoverGeometryWireframe will
      // recognize.  Note that because the data set comes from
      // UnstructuredGridGeometryFilter, the ids will represent the faces rather
      // than the original cells, which is important.
      this->DataSetSurfaceFilter->PassThroughCellIdsOn();
      this->DataSetSurfaceFilter->SetOriginalCellIdsName(
                            vtkPVRecoverGeometryWireframe::ORIGINAL_FACE_IDS());

      if (this->PassThroughPointIds)
        {
        if (this->NonlinearSubdivisionLevel <= 1)
          {
          // Do not allow the vtkDataSetSurfaceFilter create an array of
          // original cell ids; it will overwrite the correct array from the
          // vtkUnstructuredGridGeometryFilter.
          this->DataSetSurfaceFilter->PassThroughPointIdsOff();
          }
        else
          {
          // vtkDataSetSurfaceFilter is going to strip the vtkOriginalPointIds
          // created by the vtkPVUnstructuredGridGeometryFilter because it
          // cannot interpolate the ids.  Make the vtkDataSetSurfaceFilter make
          // its own original ids array.  We will resolve them later.
          this->DataSetSurfaceFilter->PassThroughPointIdsOn();
          }
        }
      }

    if (input->GetNumberOfCells() > 0)
      {
      this->DataSetSurfaceFilter->UnstructuredGridExecute(input, output);
      }

    if (handleSubdivision)
      {
      // Restore state of DataSetSurfaceFilter.
      this->DataSetSurfaceFilter->SetPassThroughCellIds(
                                                      this->PassThroughCellIds);
      this->DataSetSurfaceFilter->SetOriginalCellIdsName(NULL);
      this->DataSetSurfaceFilter->SetPassThroughPointIds(
                                                     this->PassThroughPointIds);

      // Now use vtkPVRecoverGeometryWireframe to create an edge flag attribute
      // that will cause the wireframe to be rendered correctly.
      VTK_CREATE(vtkPolyData, nextStageInput);
      nextStageInput->ShallowCopy(output);  // Yes output is correct.
      this->RecoverWireframeFilter->SetInput(nextStageInput);

      // Observe the progress of the internal filter.
      // TODO: Make the consecutive internal filter execution have monotonically
      // increasing progress rather than restarting for every internal filter.
      this->RecoverWireframeFilter->AddObserver(
                                                vtkCommand::ProgressEvent,
                                                this->InternalProgressObserver);
      this->RecoverWireframeFilter->Update();
      // The internal filter finished.  Remove the observer.
      this->RecoverWireframeFilter->RemoveObserver(
                                                this->InternalProgressObserver);

      this->RecoverWireframeFilter->SetInput(NULL);

      // Get what should be the final output.
      output->ShallowCopy(this->RecoverWireframeFilter->GetOutput());

      if (this->PassThroughPointIds && (this->NonlinearSubdivisionLevel > 1))
        {
        // The output currently has a vtkOriginalPointIds array that maps points
        // to the data containing only the faces.  Correct this to point to the
        // original data set.
        vtkIdTypeArray *polyPtIds2FacePtIds = vtkIdTypeArray::SafeDownCast(
                       output->GetPointData()->GetArray("vtkOriginalPointIds"));
        if (!facePtIds2OriginalPtIds || !polyPtIds2FacePtIds)
          {
          vtkErrorMacro(<< "Missing original point id arrays.");
          return;
          }
        vtkIdType numPts = polyPtIds2FacePtIds->GetNumberOfTuples();
        VTK_CREATE(vtkIdTypeArray, polyPtIds2OriginalPtIds);
        polyPtIds2OriginalPtIds->SetName("vtkOriginalPointIds");
        polyPtIds2OriginalPtIds->SetNumberOfComponents(1);
        polyPtIds2OriginalPtIds->SetNumberOfTuples(numPts);
        for (vtkIdType polyPtId = 0; polyPtId < numPts; polyPtId++)
          {
          vtkIdType facePtId = polyPtIds2FacePtIds->GetValue(polyPtId);
          vtkIdType originalPtId = -1;
          if (facePtId >= 0)
            {
            originalPtId = facePtIds2OriginalPtIds->GetValue(facePtId);
            }
          polyPtIds2OriginalPtIds->SetValue(polyPtId, originalPtId);
          }
        output->GetPointData()->AddArray(polyPtIds2OriginalPtIds);
        }
      }

    return;
    }

  this->OutlineFlag = 1;

  this->DataSetExecute(input, output, doCommunicate);
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PolyDataExecute(
  vtkPolyData* input, vtkPolyData* out, int doCommunicate)
{
  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;
    if (this->UseStrips)
      {
      vtkPolyData *inCopy = vtkPolyData::New();
      vtkStripper *stripper = vtkStripper::New();
      stripper->SetPassThroughCellIds(this->PassThroughCellIds);
      //stripper->SetPassThroughPointIds(this->PassThroughPointIds);
      inCopy->ShallowCopy(input);
      inCopy->RemoveGhostCells(1);
      stripper->SetInput(inCopy);
      stripper->Update();
      out->CopyStructure(stripper->GetOutput());
      out->GetPointData()->ShallowCopy(stripper->GetOutput()->GetPointData());
      out->GetCellData()->ShallowCopy(stripper->GetOutput()->GetCellData());
      inCopy->Delete();
      stripper->Delete();
      }
    else
      {
      out->ShallowCopy(input);
      if (this->PassThroughCellIds)
        {
        vtkIdTypeArray *originalCellIds = vtkIdTypeArray::New();
        originalCellIds->SetName("vtkOriginalCellIds");
        originalCellIds->SetNumberOfComponents(1);
        vtkCellData *outputCD = out->GetCellData();
        outputCD->AddArray(originalCellIds);
        vtkIdType numTup = out->GetNumberOfCells();
        originalCellIds->SetNumberOfValues(numTup);
        for (vtkIdType cId = 0; cId < numTup; cId++)
          {
          originalCellIds->SetValue(cId, cId);
          }
        originalCellIds->Delete();
        originalCellIds = NULL;
        }
      if (this->PassThroughPointIds)
        {
        vtkIdTypeArray *originalPointIds = vtkIdTypeArray::New();
        originalPointIds->SetName("vtkOriginalPointIds");
        originalPointIds->SetNumberOfComponents(1);
        vtkPointData *outputPD = out->GetPointData();
        outputPD->AddArray(originalPointIds);
        vtkIdType numTup = out->GetNumberOfPoints();
        originalPointIds->SetNumberOfValues(numTup);
        for (vtkIdType pId = 0; pId < numTup; pId++)
          {
          originalPointIds->SetValue(pId, pId);
          }
        originalPointIds->Delete();
        originalPointIds = NULL;
        }
      out->RemoveGhostCells(1);
      }
    return;
    }

  this->OutlineFlag = 1;
  this->DataSetExecute(input, out, doCommunicate);
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::OctreeExecute(
  vtkHyperOctree* input, vtkPolyData* out, int doCommunicate)
{
  if (!this->UseOutline)
    {
    this->OutlineFlag = 0;

    vtkHyperOctreeSurfaceFilter* internalFilter =
      vtkHyperOctreeSurfaceFilter::New();
    internalFilter->SetPassThroughCellIds(this->PassThroughCellIds);
    //internalFilter->SetPassThroughPointIds(this->PassThroughPointIds);
    vtkHyperOctree* octreeCopy = vtkHyperOctree::New();
    octreeCopy->ShallowCopy(input);
    internalFilter->SetInput(octreeCopy);
    internalFilter->Update();
    out->ShallowCopy(internalFilter->GetOutput());
    octreeCopy->Delete();
    internalFilter->Delete();
    return;
    }

  this->OutlineFlag = 1;
  this->DataSetExecute(input, out, doCommunicate);
}

//----------------------------------------------------------------------------
int vtkPVGeometryFilter::FillInputPortInformation(int port,
                                                  vtkInformation* info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
    {
    return 0;
    }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGenericDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVGeometryFilter::ReportReferences(vtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  vtkGarbageCollectorReport(collector, this->DataSetSurfaceFilter,
                            "DataSetSurfaceFilter");
  vtkGarbageCollectorReport(collector, this->GenericGeometryFilter,
                            "GenericGeometryFilter");
  vtkGarbageCollectorReport(collector, this->UnstructuredGridGeometryFilter,
                            "UnstructuredGridGeometryFilter");
  vtkGarbageCollectorReport(collector, this->RecoverWireframeFilter,
                            "RecoverWireframeFilter");
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->OutlineFlag)
    {
    os << indent << "OutlineFlag: On\n";
    }
  else
    {
    os << indent << "OutlineFlag: Off\n";
    }

  os << indent << "UseOutline: " << (this->UseOutline?"on":"off") << endl;
  os << indent << "UseStrips: " << (this->UseStrips?"on":"off") << endl;
  os << indent << "GenerateCellNormals: "
     << (this->GenerateCellNormals?"on":"off") << endl;
  os << indent << "NonlinearSubdivisionLevel: "
     << this->NonlinearSubdivisionLevel << endl;
  os << indent << "Controller: " << this->Controller << endl;

  os << indent << "PassThroughCellIds: "
     << (this->PassThroughCellIds ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: "
     << (this->PassThroughPointIds ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::SetPassThroughCellIds(int newvalue)
{
  this->PassThroughCellIds = newvalue;
  if (this->DataSetSurfaceFilter)
    {
    this->DataSetSurfaceFilter->SetPassThroughCellIds(
      this->PassThroughCellIds);
    }
  if (this->GenericGeometryFilter)
    {
    this->GenericGeometryFilter->SetPassThroughCellIds(
      this->PassThroughCellIds);
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::SetPassThroughPointIds(int newvalue)
{
  this->PassThroughPointIds = newvalue;
  if (this->DataSetSurfaceFilter)
    {
    this->DataSetSurfaceFilter->SetPassThroughPointIds(
      this->PassThroughPointIds);
    }
  /*
  if (this->GenericGeometryFilter)
    {
    this->GenericGeometryFilter->SetPassThroughPointIds(
      this->PassThroughPointIds);
    }
  */
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::SetForceUseStrips(int newvalue)
{
  this->ForceUseStrips = newvalue;
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::SetUseStrips(int newvalue)
{
  if (this->UseStrips != newvalue)
    {
    this->UseStrips = newvalue;
    if (this->DataSetSurfaceFilter)
      {
      this->DataSetSurfaceFilter->SetUseStrips(this->UseStrips);
      }
    //this little bit of nastiness is here for surface selection
    //surf selection has to have strips off
    //but we don't want to reexecute this filter unless we really really have
    //to, so this checks:
    //if we have been asked to change the setting for selection AND
    //if something other than the strip setting has been changed
    int OnlyStripsChanged = 1;
    if ((this->GetInput() &&
        this->GetInput()->GetMTime() > this->StripSettingMTime)
        ||
        this->MTime > this->StripSettingMTime
        ||
        this->StripModFirstPass)
        {
        OnlyStripsChanged = 0;
        }
    if (this->ForceUseStrips &&
        !OnlyStripsChanged)
      {
      this->Modified();
      this->StripModFirstPass = 0;
      }
    this->StripSettingMTime.Modified();
    }
}

//----------------------------------------------------------------------------
void vtkPVGeometryFilter::RemoveGhostCells(vtkPolyData* output)
{
  vtkDataArray* ghost = output->GetCellData()->GetArray("vtkGhostLevels");
  if (ghost)
    {
    output->RemoveGhostCells(1);
    }
}

//-----------------------------------------------------------------------------
void vtkPVGeometryFilter::SetNonlinearSubdivisionLevel(int newvalue)
{
  if (this->NonlinearSubdivisionLevel != newvalue)
    {
    this->NonlinearSubdivisionLevel = newvalue;

    if (this->DataSetSurfaceFilter)
      {
      this->DataSetSurfaceFilter->SetNonlinearSubdivisionLevel(
                                               this->NonlinearSubdivisionLevel);
      }

    this->Modified();
    }
}
