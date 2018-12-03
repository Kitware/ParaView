/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIntersectFragments.h"

#include "vtkObject.h"
#include "vtkObjectFactory.h"
// Pipeline
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
// PV interface

// Data sets
#include "vtkCompositeDataIterator.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkPolyData.h"
// Arrays & containers
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkMaterialInterfaceIdList.h"
#include "vtkMaterialInterfacePieceLoading.h"
#include "vtkMaterialInterfacePieceTransaction.h"
#include "vtkMaterialInterfacePieceTransactionMatrix.h"
#include "vtkMaterialInterfaceProcessLoading.h"
#include "vtkMaterialInterfaceProcessRing.h"
#include "vtkMaterialInterfaceToProcMap.h"
#include "vtkPointAccumulator.h"
#include "vtkPointData.h"
#include "vtkUnsignedIntArray.h"
// Io/Ipc
#include "vtkMaterialInterfaceCommBuffer.h"
// Filters etc...
#include "vtkCutter.h"
#include "vtkImplicitFunction.h"
#include "vtkPlane.h"
// STL
#include <sstream>
using std::ostringstream;
#include <vector>
using std::vector;
#include <string>
using std::string;
#include "algorithm"
// ansi c
#include <math.h>
// other
#include "vtkMaterialInterfaceUtilities.h"

vtkStandardNewMacro(vtkIntersectFragments);

//----------------------------------------------------------------------------
vtkIntersectFragments::vtkIntersectFragments()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);

  this->Controller = vtkMultiProcessController::GetGlobalController();

  this->Cutter = vtkCutter::New();
  this->CutFunction = 0;

  this->GeomIn = 0;
  this->GeomOut = 0;
  this->StatsIn = 0;
  this->StatsOut = 0;
  this->NBlocks = 0;

  this->Progress = 0.0;
}

//----------------------------------------------------------------------------
vtkIntersectFragments::~vtkIntersectFragments()
{
  this->Controller = 0;
  ClearVectorOfVtkPointers(this->IntersectionCenters);
  CheckAndReleaseVtkPointer(this->Cutter);
  this->SetCutFunction(0);
}

//----------------------------------------------------------------------------
// Input:
// 0 is expected to be distributed fragment geometry
// 1 is expected to be fragment statistics on proc 0
int vtkIntersectFragments::FillInputPortInformation(int /*port*/, vtkInformation* info)
{
  // both inputs are mulitblock
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");

  return 1;
}

//----------------------------------------------------------------------------
// Output:
// 0 intersection statistics on proc 0
// 1 intersection geometry, distributed
int vtkIntersectFragments::FillOutputPortInformation(int port, vtkInformation* info)
{
  // There are two outputs,
  // 0: multi-block contains fragment's geom
  // 1: multiblock contains fragment's centers with attributes
  switch (port)
  {
    case 0:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    case 1:
      info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet");
      break;
    default:
      assert(0 && "Invalid output port.");
      break;
  }

  return 1;
}

//----------------------------------------------------------------------------
// Connect pipeline
void vtkIntersectFragments::SetGeometryInputConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
// Connect pipeline
void vtkIntersectFragments::SetStatisticsInputConnection(vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Make a destination data set with the same structure as the
// source. The sources are expected to be multi block of either
// polydata or multipiece of polydata.
//
// return 0 if we couldn't copy the structure.
int vtkIntersectFragments::CopyInputStructureStats(
  vtkMultiBlockDataSet* dest, vtkMultiBlockDataSet* src)
{
  assert("Unexpected number of blocks in the statistics input." &&
    (unsigned int)this->NBlocks == src->GetNumberOfBlocks());

  dest->SetNumberOfBlocks(this->NBlocks);

  // do we have an empty data set??
  if (this->NBlocks == 0)
  {
    return 0;
  }
  // copy point data structure, to get the names
  // and numbers of components.
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    vtkPolyData* srcPd = dynamic_cast<vtkPolyData*>(src->GetBlock(blockId));
    // stats exists on only one proc if we find empty block
    // assume this is not it. Its not an error.
    if (srcPd == 0)
    {
      break;
    }
    vtkPolyData* destPd = vtkPolyData::New();
    destPd->GetPointData()->CopyStructure(srcPd->GetPointData());
    dest->SetBlock(blockId, destPd);
    destPd->Delete();
  }
  return 1;
}

//----------------------------------------------------------------------------
// Make a destination data set with the same structure as the
// source. The sources are expected to be multi block of either
// polydata or multipiece of polydata.
//
// return 0 if we couldn't copy the structure.
int vtkIntersectFragments::CopyInputStructureGeom(
  vtkMultiBlockDataSet* dest, vtkMultiBlockDataSet* src)
{
  dest->SetNumberOfBlocks(this->NBlocks);

  // do we have an empty data set??
  if (this->NBlocks == 0)
  {
    return 0;
  }

  // for non-empty data sets we expect that all
  // blocks are multipiece of polydata (geom)
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    vtkMultiPieceDataSet* srcFragments =
      dynamic_cast<vtkMultiPieceDataSet*>(src->GetBlock(blockId));

    if (srcFragments != 0)
    {
      vtkMultiPieceDataSet* destFragments = vtkMultiPieceDataSet::New();
      int nSrcFragments = srcFragments->GetNumberOfPieces();
      destFragments->SetNumberOfPieces(nSrcFragments);
      dest->SetBlock(blockId, destFragments);
      destFragments->Delete();
#ifdef vtkIntersectFragmentsDEBUG
      cerr << "[" << __LINE__ << "]"
           << "[" << this->Controller->GetLocalProcessId() << "]"
           << "Input block " << blockId << " has " << nSrcFragments << " fragments." << endl;
#endif
    }
    else
    {
      assert("Unexpected input structure." && blockId == 0);
      vtkErrorMacro("Unexpected input structure.");
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
// Build a list of fragment ids which we own. The id is the
// index into the piece where the fragment lives in the input.
//
// return 0 on error.
int vtkIntersectFragments::IdentifyLocalFragments()
{
  int nProcs = this->Controller->GetNumberOfProcesses();
  this->FragmentIds.clear();
  this->FragmentIds.resize(this->NBlocks);
  // look in each material
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    vtkMultiPieceDataSet* fragments =
      dynamic_cast<vtkMultiPieceDataSet*>(this->GeomIn->GetBlock(blockId));
    assert("Could not get fragments." && fragments);
    int nFragments = fragments->GetNumberOfPieces();
    this->FragmentIds[blockId].reserve(nFragments / nProcs);
    // All of the inputs have the same structure. If the
    // entry is 0 then we don't have any of the fragment's
    // geometry.
    for (int fragmentId = 0; fragmentId < nFragments; ++fragmentId)
    {
      vtkPolyData* fragment = dynamic_cast<vtkPolyData*>(fragments->GetPiece(fragmentId));
      if (fragment != 0)
      {
        // We have some of this fragment's geometry. Save
        // it's id. Id's are global within a block.
        this->FragmentIds[blockId].push_back(fragmentId);
      }
    }
    // free extra memory
    vector<int>(this->FragmentIds[blockId]).swap(this->FragmentIds[blockId]);
  }

#ifdef vtkIntersectFragmentsDEBUG
  cerr << "[" << __LINE__ << "]"
       << "[" << this->Controller->GetLocalProcessId() << "]"
       << "found local ids:" << endl
       << this->FragmentIds << endl;
#endif

  return 1;
}

//----------------------------------------------------------------------------
// Probe the input, configure internals and output accordingly
//
// return 0 on error.
int vtkIntersectFragments::PrepareToProcessRequest()
{
  // containers hold arrays for each block
  this->NBlocks = this->GeomIn->GetNumberOfBlocks();
  // size containers
  ResizeVectorOfVtkArrayPointers(this->IntersectionCenters, 3, 0, "centers", this->NBlocks);
  this->IntersectionIds.resize(this->NBlocks);
  // prepare the output data sets
  if ((this->CopyInputStructureGeom(this->GeomOut, this->GeomIn) == 0) ||
    (this->CopyInputStructureStats(this->StatsOut, this->StatsIn) == 0))
  {
    vtkErrorMacro("Unexpected input structure.");
    return 0;
  }
  // Find out who we have with us.
  this->IdentifyLocalFragments();
  // configure the cutter
  this->Cutter->SetCutFunction(this->CutFunction);
  // other
  this->Progress = 0.0;
  this->ProgressIncrement = 0.75 / (double)this->NBlocks;

  return 1;
}

//----------------------------------------------------------------------------
// Take the intersection of all fragments we own with the
// specified implicit function.
//
// return 0 on error
int vtkIntersectFragments::Intersect()
{
  // look in each material
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    vector<int>& ids = this->IntersectionIds[blockId];
    vtkMultiPieceDataSet* geomOut =
      dynamic_cast<vtkMultiPieceDataSet*>(this->GeomOut->GetBlock(blockId));
    // for fragments we own
    vtkMultiPieceDataSet* fragments =
      dynamic_cast<vtkMultiPieceDataSet*>(this->GeomIn->GetBlock(blockId));
    vector<int>& fragmentIds = this->FragmentIds[blockId];
    size_t nLocal = fragmentIds.size();
    for (size_t localId = 0; localId < nLocal; ++localId)
    {
      int globalId = fragmentIds[localId];
      vtkPolyData* fragment = dynamic_cast<vtkPolyData*>(fragments->GetPiece(globalId));
      // cut
      this->Cutter->SetInputData(fragment);
      this->Cutter->Update();
      vtkPolyData* intersection = this->Cutter->GetOutput();

      if (intersection->GetNumberOfPoints() > 0)
      {
        ids.push_back(globalId);
        // pass intersection geometry
        vtkPolyData* intersectionOut = vtkPolyData::New();
        intersectionOut->ShallowCopy(intersection);
        geomOut->SetPiece(globalId, intersectionOut);
        intersectionOut->Delete();
      }
    }
    // free extra memory
    vector<int>(ids).swap(ids);

    this->Progress += this->ProgressIncrement;
    this->UpdateProgress(this->Progress);
  }

#ifdef vtkIntersectFragmentsDEBUG
  cerr << "[" << __LINE__ << "]"
       << "[" << this->Controller->GetLocalProcessId() << "]"
       << "intersection produced:" << endl
       << this->IntersectionIds;
#endif
  return 1;
}

//----------------------------------------------------------------------------
// Build the loading array for the fragment pieces that we own.
void vtkIntersectFragments::BuildLoadingArray(vector<vtkIdType>& loadingArray, int blockId)
{
  vtkMultiPieceDataSet* intersectGeometry =
    dynamic_cast<vtkMultiPieceDataSet*>(this->GeomOut->GetBlock(blockId));
  int nFragments = intersectGeometry->GetNumberOfPieces();

  size_t nLocal = this->IntersectionIds[blockId].size();

  loadingArray.clear();
  loadingArray.resize(nFragments, 0);
  for (size_t localId = 0; localId < nLocal; ++localId)
  {
    int globalId = this->IntersectionIds[blockId][localId];

    vtkPolyData* geom = dynamic_cast<vtkPolyData*>(intersectGeometry->GetPiece(globalId));

    loadingArray[globalId] = geom->GetNumberOfCells();
  }
}

//----------------------------------------------------------------------------
// Load a buffer containing the number of polys for each fragment
// or fragment piece that we own. Return the size in vtkIdType's
// of the packed buffer and the buffer itself. Pass in a
// pointer initialized to null, allocation is internal.
int vtkIntersectFragments::PackLoadingArray(vtkIdType*& buffer, int blockId)
{
  assert("Buffer appears to have been pre-allocated." && buffer == 0);

  vtkMultiPieceDataSet* intersectGeometry =
    dynamic_cast<vtkMultiPieceDataSet*>(this->GeomOut->GetBlock(blockId));

  size_t nLocal = this->IntersectionIds[blockId].size();

  vtkMaterialInterfacePieceLoading pl;
  const int bufSize = static_cast<int>(pl.SIZE * nLocal);
  buffer = new vtkIdType[bufSize];
  vtkIdType* pBuf = buffer;
  for (size_t localId = 0; localId < nLocal; ++localId)
  {
    int globalId = this->IntersectionIds[blockId][localId];

    vtkPolyData* geom = dynamic_cast<vtkPolyData*>(intersectGeometry->GetPiece(globalId));

    pl.Initialize(globalId, geom->GetNumberOfCells());
    pl.Pack(pBuf);
    pBuf += pl.SIZE;
  }

  return bufSize;
}

//----------------------------------------------------------------------------
// Given a fragment loading array that has been packed into an int array
// unpack. The packed loading arrays are ordered locally while the unpacked
// are ordered gloably by fragment id.
//
// Return the number of fragments and the unpacked array.
int vtkIntersectFragments::UnPackLoadingArray(
  vtkIdType* buffer, int bufSize, vector<vtkIdType>& loadingArray, int blockId)
{
  const int sizeOfPl = vtkMaterialInterfacePieceLoading::SIZE;

  assert("Buffer is null pointer." && buffer != 0);
  assert("Buffer size is incorrect." && bufSize % sizeOfPl == 0);

  vtkMultiPieceDataSet* intersectGeometry =
    dynamic_cast<vtkMultiPieceDataSet*>(this->GeomOut->GetBlock(blockId));
  int nFragments = intersectGeometry->GetNumberOfPieces();

  loadingArray.clear();
  loadingArray.resize(nFragments, 0);
  vtkIdType* pBuf = buffer;
  const int nPieces = bufSize / sizeOfPl;
  for (int i = 0; i < nPieces; ++i)
  {
    vtkMaterialInterfacePieceLoading pil;
    pil.UnPack(pBuf);
    loadingArray[pil.GetId()] = pil.GetLoading();
    pBuf += sizeOfPl;
  }

  return nPieces;
}

//----------------------------------------------------------------------------
// Identify intersection geometry that is split across processes
// and compute geometric attributes. For fragment intersections
// whose geometry is split across processes, first localize points
// and compute geometric attributes. eg OBB, AABB center.
void vtkIntersectFragments::ComputeGeometricAttributes()
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  vtkCommunicator* comm = this->Controller->GetCommunicator();
  const int controllingProcId = 0;
  const int msgBase = 200000;

  if (myProcId == controllingProcId)
  {
    // prepare to count number of fragments we hit in
    // each block
    this->NFragmentsIntersected.clear();
    this->NFragmentsIntersected.resize(this->NBlocks, 0);
  }

  // Make a pass for each block. Note: We could reduce communication
  // overhead by sending all blocks at once. probably not
  // worth the added complication and effort.
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    // Here are the intersection pieces we own. Some are completely
    // localized while others are split across processes. Pieces
    // with split geometry will be temporarily localized(via copy), the
    // geometry acted on as a whole and the results distributed back
    // to piece owners. For localized intersections(these aren't pieces)
    // the situation is relatively simple--computation can be made
    // directly.
    vtkMultiPieceDataSet* intersectGeometry =
      dynamic_cast<vtkMultiPieceDataSet*>(this->GeomOut->GetBlock(blockId));
    int nFragments = intersectGeometry->GetNumberOfPieces();

    vector<int>& globalIds = this->IntersectionIds[blockId];
    size_t nLocal = globalIds.size();

    // Start by assuming that all intersection geometry
    // are local. If we are are running client-server
    // we search for split geometry and mark them
    // Un-marked fragments geometric attributes are
    // computed directly.
    vector<int> intersectSplitMarker;
    // intilize to (bool)0 not split. All local ops
    // on fragments check here. It's copied to geometry.
    intersectSplitMarker.resize(nLocal, 0);
    // Size the attribute arrays
    this->IntersectionCenters[blockId]->SetNumberOfComponents(3);
    this->IntersectionCenters[blockId]->SetNumberOfTuples(static_cast<vtkIdType>(nLocal));
    // Prepare for projection onto the cut function
    double r0[3], N[3];
    vtkPlane* plane = dynamic_cast<vtkPlane*>(this->CutFunction);
    if (plane != 0)
    {
      // Get a point on the plane and its
      // unit normal.
      plane->GetOrigin(r0);
      plane->GetNormal(N);
      double modN = 0;
      for (int q = 0; q < 3; ++q)
      {
        modN += N[q] * N[q];
      }
      modN = sqrt(modN);
      for (int q = 0; q < 3; ++q)
      {
        N[q] = N[q] / modN;
      }
    }

    if (nProcs == 1)
    {
      this->NFragmentsIntersected[blockId] = static_cast<int>(nLocal);
    }
    // Here we localize fragment geomtery that is split across processes.
    // A controlling process gathers structural information regarding
    // the fragment to process distribution, creates a blue print of the
    // moves that must occur to place the split pieces on a single process.
    // These moves are made, computations made then results are sent
    // back to piece owners.
    else // if (nProcs>1)
    {
      vtkMaterialInterfacePieceTransactionMatrix TM;
      TM.Initialize(nFragments, nProcs);
      // controller receives loading information and
      // builds the transaction matrix.
      if (myProcId == controllingProcId)
      {
        int thisMsgId = msgBase;
        // fragment indexed arrays, with number of polys
        vector<vector<vtkIdType> > loadingArrays;
        loadingArrays.resize(nProcs);
        // Total number of fragments we hit
        this->NFragmentsIntersected[blockId] = 0;
        // Gather loading arrays
        // mine
        this->BuildLoadingArray(loadingArrays[controllingProcId], blockId);
        // others
        for (int procId = 0; procId < nProcs; ++procId)
        {
          if (procId == controllingProcId)
          {
            continue;
          }
          // size of incoming
          int bufSize = 0;
          comm->Receive(&bufSize, 1, procId, thisMsgId);
          // incoming
          vtkIdType* buffer = new vtkIdType[bufSize];
          comm->Receive(buffer, bufSize, procId, thisMsgId + 1);
          this->UnPackLoadingArray(buffer, bufSize, loadingArrays[procId], blockId);
          delete[] buffer;
        }
        ++thisMsgId;
        ++thisMsgId;
#ifdef vtkIntersectFragmentsDEBUG
        cerr << "[" << __LINE__ << "] " << controllingProcId << " loading histogram:" << endl;
        PrintPieceLoadingHistogram(loadingArrays);
#endif
        // Build fragment to proc map
        vtkMaterialInterfaceToProcMap f2pm;
        f2pm.Initialize(nProcs, nFragments);
        for (int procId = 0; procId < nProcs; ++procId)
        {
          // sum up the loading contribution from all local
          // intersection geometry and make a note of who owns what.
          for (int fragmentId = 0; fragmentId < nFragments; ++fragmentId)
          {
            vtkIdType loading = loadingArrays[procId][fragmentId];
            if (loading > 0)
            {
              f2pm.SetProcOwnsPiece(procId, fragmentId);
            }
          }
        }
#ifdef vtkIntersectFragmentsDEBUG
        vector<int> splitting(nProcs + 1, 0);
        int nSplit = 0;
#endif
        // Who will do attribute processing for fragments
        // that are split? Cycle through the processes
        // assigning to each until all work has been allotted.
        vtkMaterialInterfaceProcessRing procRing;
        procRing.Initialize(nProcs);
        // Decide who needs to move and build corresponding
        // transaction matrix.
        for (int fragmentId = 0; fragmentId < nFragments; ++fragmentId)
        {
          int nSplitOver = f2pm.GetProcCount(fragmentId);
          // Count the number of fragments we hit.
          if (nSplitOver > 0)
          {
            ++this->NFragmentsIntersected[blockId];
          }
          // if the intersection is split then we need to
          // copy it's points
          if (nSplitOver > 1)
          {
#ifdef vtkIntersectFragmentsDEBUG
            // splitting histogram
            ++splitting[nSplitOver];
            ++nSplit;
#endif
            // who has the pieces?
            vector<int> owners = f2pm.WhoHasAPiece(fragmentId);
            // who will do processing??
            int recipient = procRing.GetNextId();
            // Add the transactions to make the copy
            vtkMaterialInterfacePieceTransaction ta;
            for (int i = 0; i < nSplitOver; ++i)
            {
              // need to move this piece?
              if (owners[i] == recipient)
              {
                continue;
              }
              // Add the requisite transactions.
              // recipient executes a recv from owner
              ta.Initialize('R', owners[i]);
              TM.PushTransaction(fragmentId, recipient, ta);
              // owner executes a send to recipient
              ta.Initialize('S', recipient);
              TM.PushTransaction(fragmentId, owners[i], ta);
            }
          }
        }
#ifdef vtkIntersectFragmentsDEBUG
        cerr << "[" << __LINE__ << "] " << controllingProcId << " splitting:" << endl;
        PrintHistogram(splitting);
        cerr << "[" << __LINE__ << "] " << myProcId << " total number of fragments " << nFragments
             << endl;
        cerr << "[" << __LINE__ << "] " << myProcId << " total number split " << nSplit << endl;
        cerr << "[" << __LINE__ << "] " << myProcId
             << " Number of fragments intersected: " << this->NFragmentsIntersected << endl;
        cerr << "[" << __LINE__ << "] " << myProcId << " the transaction matrix is:" << endl;
        TM.Print();
#endif
      }
      // All processes send fragment loading to controller.
      else
      {
        int thisMsgId = msgBase;
        // create and send my loading array
        vtkIdType* buffer = 0;
        int bufSize = this->PackLoadingArray(buffer, blockId);
        comm->Send(&bufSize, 1, controllingProcId, thisMsgId);
        ++thisMsgId;
        comm->Send(buffer, bufSize, controllingProcId, thisMsgId);
        ++thisMsgId;
      }

      // Broadcast the transaction matrix
      TM.Broadcast(comm, controllingProcId);
      // Prepare for inverse look of fragment ids
      vtkMaterialInterfaceIdList idList;
      idList.Initialize(this->IntersectionIds[blockId], true);

      // localize split geometry and compute attributes.
      for (int fragmentId = 0; fragmentId < nFragments; ++fragmentId)
      {
        // point buffer
        vtkPointAccumulator<float, vtkFloatArray> accumulator;
        // get my list of transactions for this fragment
        vector<vtkMaterialInterfacePieceTransaction>& transactionList =
          TM.GetTransactions(fragmentId, myProcId);
        // execute
        vtkPolyData* localMesh =
          dynamic_cast<vtkPolyData*>(intersectGeometry->GetPiece(fragmentId));

        size_t nTransactions = transactionList.size();
        if (nTransactions > 0)
        {
          /// send
          if (transactionList[0].GetType() == 'S')
          {
            assert("Send has more than 1 transaction." && nTransactions == 1);
            assert("Send requires a mesh that is not local." && localMesh != 0);

            // I am sending geometry, hence this is a piece
            // of a split intersection and I need to treat it as
            // such from now on.
            int localId = idList.GetLocalId(fragmentId);
            assert("Fragment id not found." && localId != -1);
            intersectSplitMarker[localId] = 1;

            // Send the geometry and recvieve the results of
            // the requested computations.
            vtkMaterialInterfacePieceTransaction& ta = transactionList[0];

            // get the points of this piece
            vtkFloatArray* ptsArray =
              dynamic_cast<vtkFloatArray*>(localMesh->GetPoints()->GetData());
            const vtkIdType bytesPerPoint = 3 * sizeof(float);
            const vtkIdType nPoints = ptsArray->GetNumberOfTuples();
            const vtkIdType bufferSize = bytesPerPoint * nPoints;
            // prepare a comm buffer
            vtkMaterialInterfaceCommBuffer buffer;
            buffer.Initialize(myProcId, 1, bufferSize);
            buffer.SetNumberOfTuples(0, nPoints);
            buffer.Pack(ptsArray);
            // send the buffer in two parts, first the header
            this->Controller->Send(
              buffer.GetHeader(), buffer.GetHeaderSize(), ta.GetRemoteProc(), fragmentId);
            // followed by points
            this->Controller->Send(
              buffer.GetBuffer(), buffer.GetBufferSize(), ta.GetRemoteProc(), 2 * fragmentId);

            // Receive the remotely computed attributes
            double aabbCen[3];
            this->Controller->Receive(aabbCen, 3, ta.GetRemoteProc(), 3 * fragmentId);
            // save results
            this->IntersectionCenters[blockId]->SetTuple(localId, aabbCen);
          }
          /// receive
          else if (transactionList[0].GetType() == 'R')
          {
            // This fragment is split across processes and
            // I have a piece. From now on I need to treat
            // this fragment as split.
            int localId = -1;
            if (localMesh != 0)
            {
              localId = idList.GetLocalId(fragmentId);
              assert("Fragment id not found." && localId != -1);
              intersectSplitMarker[localId] = 1;
            }

            // Receive the geometry, perform the requested
            // computations and send back the results.
            for (size_t i = 0; i < nTransactions; ++i)
            {
              vtkMaterialInterfacePieceTransaction& ta = transactionList[i];

              // prepare the comm buffer to receive attribute data
              // pertaining to a single block(material)
              vtkMaterialInterfaceCommBuffer buffer;
              buffer.SizeHeader(1);
              // receive buffer's header
              this->Controller->Receive(
                buffer.GetHeader(), buffer.GetHeaderSize(), ta.GetRemoteProc(), fragmentId);
              // size buffer via incoming header
              buffer.SizeBuffer();
              // receive points
              this->Controller->Receive(
                buffer.GetBuffer(), buffer.GetBufferSize(), ta.GetRemoteProc(), 2 * fragmentId);
              // unpack points with an explicit copy
              vtkIdType nPoints = buffer.GetNumberOfTuples(0);
              float* writePointer = accumulator.Expand(nPoints);
              buffer.UnPack(writePointer, 3, nPoints, true);
            }
            // append points that I own.
            if (localMesh != 0)
            {
              // get the points
              vtkFloatArray* ptsArray =
                dynamic_cast<vtkFloatArray*>(localMesh->GetPoints()->GetData());
              // append
              accumulator.Accumulate(ptsArray);
            }

            // Get the gathered points in vtk form.
            vtkPoints* localizedPoints = accumulator.BuildVtkPoints();
            // Get the AABB and compute its center.
            double aabb[6];
            localizedPoints->GetBounds(aabb);
            double aabbCen[3];
            for (int q = 0, k = 0; q < 3; ++q, k += 2)
            {
              aabbCen[q] = (aabb[k] + aabb[k + 1]) / 2.0;
            }
            // project back onto the plane
            if (plane != 0)
            {
              double d = 0;
              for (int q = 0; q < 3; ++q)
              {
                d += N[q] * (aabbCen[q] - r0[q]);
              }
              for (int q = 0; q < 3; ++q)
              {
                aabbCen[q] -= N[q] * d;
              }
            }

            // send attributes back to piece owners
            for (size_t i = 0; i < nTransactions; ++i)
            {
              vtkMaterialInterfacePieceTransaction& ta = transactionList[i];
              this->Controller->Send(aabbCen, 3, ta.GetRemoteProc(), 3 * fragmentId);
            }
            // If I own a piece save the results, and mark
            // piece as split.
            if (localMesh != 0)
            {
              this->IntersectionCenters[blockId]->SetTuple(localId, aabbCen);
            }
            // Clean up.
            localizedPoints->Delete();
            accumulator.Clear();
          }
          else
          {
            assert("Invalid transaction type." && 0);
          }
        }
      }
    }

    // At this poiint we have identified all split frafgments
    // tenmporarily localized their geometry, computed the attributes
    // and sent results back to piece owners. Now compute geometric
    // attributes for remianing local fragments.
    double aabb[6];
    double* pCoaabb = this->IntersectionCenters[blockId]->GetPointer(0);

    // Traverse the fragments we own
    for (size_t localId = 0; localId < nLocal; ++localId)
    {
      // skip fragments with geometry split over multiple
      // processes. These have been already taken care of.
      if (intersectSplitMarker[localId] == 1)
      {
        pCoaabb += 3;
        continue;
      }

      int globalId = globalIds[localId];

      vtkPolyData* thisFragment = dynamic_cast<vtkPolyData*>(intersectGeometry->GetPiece(globalId));

      // Center of AABB calculation
      thisFragment->GetBounds(aabb);
      for (int q = 0, k = 0; q < 3; ++q, k += 2)
      {
        pCoaabb[q] = (aabb[k] + aabb[k + 1]) / 2.0;
      }
      // project back onto the plane
      if (plane != 0)
      {
        double d = 0;
        for (int q = 0; q < 3; ++q)
        {
          d += N[q] * (pCoaabb[q] - r0[q]);
        }
        for (int q = 0; q < 3; ++q)
        {
          pCoaabb[q] -= N[q] * d;
        }
      }
      pCoaabb += 3;
    } // fragment traversal
  }

#ifdef vtkIntersectFragmentsDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId << " intersect centers: " << this->IntersectionCenters
       << endl
       << "Computation of geometric attributes completed." << endl;
#endif
}

//----------------------------------------------------------------------------
// Receive all geomteric attribute arrays from all other
// processes. Containers filled with the expected number
// of empty data arrays/pointers are expected.
//
// The following structure is expected:
// attribute[nProcs][nMaterials]
// buffers[nProcs]
//
// return 0 on error
int vtkIntersectFragments::CollectGeometricAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vector<vtkDoubleArray*> >& centers,
  vector<vector<int*> >& ids)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();
  const int msgBase = 200000;

  // size header.
  vtkMaterialInterfaceCommBuffer::SizeHeader(buffers, this->NBlocks);

  // gather
  for (int procId = 0; procId < nProcs; ++procId)
  {
    // skip mine
    if (procId == myProcId)
    {
      continue;
    }
    int thisMsgId = msgBase;
    // receive header
    this->Controller->Receive(
      buffers[procId].GetHeader(), buffers[procId].GetHeaderSize(), procId, thisMsgId);
    ++thisMsgId;
    // size buffer (recvd its size in header)
    buffers[procId].SizeBuffer();
    // receive buffer
    this->Controller->Receive(
      buffers[procId].GetBuffer(), buffers[procId].GetBufferSize(), procId, thisMsgId);
    ++thisMsgId;
    // unpack buffer
    for (int blockId = 0; blockId < this->NBlocks; ++blockId)
    {
      int nFragments = buffers[procId].GetNumberOfTuples(blockId);
      // centers, memory managed by comm buffer.
      buffers[procId].UnPack(centers[procId][blockId], 3, nFragments, false);
      // ids, memory managed by comm buffer.
      buffers[procId].UnPack(ids[procId][blockId], 1, nFragments, false);
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
// Send all geometric attributes for the fragments which I own
// to another process.
//
// return 0 on error
int vtkIntersectFragments::SendGeometricAttributes(const int recipientProcId)
{
  const int msgBase = 200000;

  const int nCompsPerBlock = 3; // centers
  vector<int> nFragments(this->NBlocks);

  // size buffer & initialize header
  vtkMaterialInterfaceCommBuffer buffer;
  buffer.SizeHeader(this->NBlocks);
  int nBytes = 0;
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    nFragments[blockId] = static_cast<int>(this->IntersectionIds[blockId].size());
    nBytes // attributes(double) + ids(int)
      += nFragments[blockId] * (nCompsPerBlock * sizeof(double) + sizeof(int));
    buffer.SetNumberOfTuples(blockId, nFragments[blockId]);
  }
  buffer.SizeBuffer(nBytes);

  // pack attributes & ids
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    // centers
    buffer.Pack(this->IntersectionCenters[blockId]);
    // ids
    buffer.Pack(&this->IntersectionIds[blockId][0], 1, nFragments[blockId]);
  }

  // send
  int thisMsgId = msgBase;
  // header
  this->Controller->Send(buffer.GetHeader(), buffer.GetHeaderSize(), recipientProcId, thisMsgId);
  ++thisMsgId;
  // buffer
  this->Controller->Send(buffer.GetBuffer(), buffer.GetBufferSize(), recipientProcId, thisMsgId);
  ++thisMsgId;

  return 1;
}

//------------------------------------------------------------------------------
// Configure buffers and containers, and put our data in.
// attribute[nProc][nMaterial]->array
//
// return 0 on error
int vtkIntersectFragments::PrepareToCollectGeometricAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vector<vtkDoubleArray*> >& centers,
  vector<vector<int*> >& ids)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // buffers
  buffers.resize(nProcs);
  // centers
  centers.resize(nProcs);
  for (int procId = 0; procId < nProcs; ++procId)
  {
    if (myProcId == procId)
    {
      centers[procId] = this->IntersectionCenters;
    }
    else
    {
      ResizeVectorOfVtkPointers(centers[procId], this->NBlocks);
    }
  }
  // ids
  ids.resize(nProcs);
  for (int procId = 0; procId < nProcs; ++procId)
  {
    ids[procId].resize(this->NBlocks, static_cast<int*>(0));
    //
    if (procId == myProcId)
    {
      for (int blockId = 0; blockId < this->NBlocks; ++blockId)
      {
        // Because we are going to us IntersectionIds to
        // hold the merged ids we copy our ids to a temp array.
        int nCenters = static_cast<int>(this->IntersectionIds[blockId].size());
        ids[myProcId][blockId] = new int[nCenters];
        for (int i = 0; i < nCenters; ++i)
        {
          ids[myProcId][blockId][i] = this->IntersectionIds[blockId][i];
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// Configure buffers and containers, and put our data in.
//
// return 0 on error
int vtkIntersectFragments::CleanUpAfterCollectGeometricAttributes(
  vector<vtkMaterialInterfaceCommBuffer>& buffers, vector<vector<vtkDoubleArray*> >& centers,
  vector<vector<int*> >& ids)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  // centers
  for (int procId = 0; procId < nProcs; ++procId)
  {
    ClearVectorOfVtkPointers(centers[procId]);
  }
  // ids
  // clean up local temp array. Remote procs are
  // using the comm buffers, they are managed there.
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    delete[] ids[myProcId][blockId];
  }
  ids.clear();
  // buffers
  buffers.clear();

  return 1;
}

//----------------------------------------------------------------------------
// Look at an incoming data array and size local arrays in
// preparation for merge. These need to be large enough to
// hold data from all processes.
//
// return 0 on error.
int vtkIntersectFragments::PrepareToMergeGeometricAttributes(vector<vector<int> >& unique)
{
  //
  unique.clear();
  unique.resize(this->NBlocks);
  // size gathered arrays
  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    int nUnique = this->NFragmentsIntersected[blockId];
    // centers
    NewVtkArrayPointer(this->IntersectionCenters[blockId], 3, nUnique,
      this->IntersectionCenters[blockId]->GetName());
    // ids
    this->IntersectionIds[blockId].resize(nUnique);
    // unique geometry marks
    vtkMultiPieceDataSet* intersectGeometry =
      dynamic_cast<vtkMultiPieceDataSet*>(this->GeomOut->GetBlock(blockId));
    int nFragments = intersectGeometry->GetNumberOfPieces();
    unique[blockId].resize(nFragments, 1);
  }
  return 1;
}
//----------------------------------------------------------------------------
// Gather all geometric attributes to a single process.
//
// return 0 on error
int vtkIntersectFragments::GatherGeometricAttributes(const int recipientProcId)
{
  const int myProcId = this->Controller->GetLocalProcessId();
  const int nProcs = this->Controller->GetNumberOfProcesses();

  if (myProcId == recipientProcId)
  {
    // collect
    vector<vtkMaterialInterfaceCommBuffer> buffers;
    vector<vector<vtkDoubleArray*> > centers;
    vector<vector<int*> > ids;
    this->PrepareToCollectGeometricAttributes(buffers, centers, ids);
    this->CollectGeometricAttributes(buffers, centers, ids);
    // merge
    vector<vector<int> > unique;
    this->PrepareToMergeGeometricAttributes(unique);
    vector<int> mergedIdx(this->NBlocks, 0); // counts merged so far
    for (int procId = 0; procId < nProcs; ++procId)
    {
      for (int blockId = 0; blockId < this->NBlocks; ++blockId)
      {
        const int idx = mergedIdx[blockId];
        const int nToMerge = centers[procId][blockId]->GetNumberOfTuples();
        // locate centers
        const int nCenterComps = 3;
        const double* pRemoteCenters = centers[procId][blockId]->GetPointer(0);
        double* pLocalCenters = this->IntersectionCenters[blockId]->GetPointer(idx * nCenterComps);
        // locate ids
        const int* pRemoteIds = ids[procId][blockId];
        // merge
        int nMerged = 0;
        for (int i = 0; i < nToMerge; ++i)
        {
          // guard against duplicates from split geometry
          if (unique[blockId][pRemoteIds[0]])
          {
            // mark as hit
            unique[blockId][pRemoteIds[0]] = 0;
            // copy centers
            for (int q = 0; q < nCenterComps; ++q)
            {
              pLocalCenters[q] = pRemoteCenters[q];
            }
            pLocalCenters += nCenterComps;
            // copy ids
            this->IntersectionIds[blockId][idx + nMerged] = pRemoteIds[0];
            ++nMerged;
          }
          // advance to next candidate to merge
          pRemoteCenters += nCenterComps;
          ++pRemoteIds;
        }
        mergedIdx[blockId] += nMerged;
      }
    }
    this->CleanUpAfterCollectGeometricAttributes(buffers, centers, ids);
    unique.clear();
  }
  else
  {
    this->SendGeometricAttributes(recipientProcId);
  }

#ifdef vtkIntersectFragmentsDEBUG
  cerr << "[" << __LINE__ << "] " << myProcId << " intersect centers: " << this->IntersectionCenters
       << endl;
#endif
  return 1;
}
//----------------------------------------------------------------------------
int vtkIntersectFragments::CopyAttributesToStatsOutput(const int controllingProcId)
{
  const int myProcId = this->Controller->GetLocalProcessId();

  if (myProcId != controllingProcId)
  {
    return 1;
  }

#ifdef vtkIntersectFragmentsDEBUG
  cerr << "[" << __LINE__ << "]"
       << "[" << myProcId << "]"
       << "copying to stats output:" << this->IntersectionCenters << this->IntersectionIds;
#endif

  for (int blockId = 0; blockId < this->NBlocks; ++blockId)
  {
    vtkPolyData* statsPd = dynamic_cast<vtkPolyData*>(this->StatsOut->GetBlock(blockId));

    const vtkIdType nCenters = this->IntersectionCenters[blockId]->GetNumberOfTuples();
    // add points and vertices.
    vtkIdTypeArray* va = vtkIdTypeArray::New();
    va->SetNumberOfTuples(2 * nCenters);
    vtkIdType* verts = va->GetPointer(0);
    vtkPoints* pts = vtkPoints::New();
    pts->SetData(this->IntersectionCenters[blockId]);
    for (int i = 0; i < nCenters; ++i)
    {
      verts[0] = 1;
      verts[1] = i;
      verts += 2;
    }
    statsPd->SetPoints(pts);
    pts->Delete();
    vtkCellArray* cells = vtkCellArray::New();
    cells->SetCells(nCenters, va);
    statsPd->SetVerts(cells);
    cells->Delete();
    va->Delete();
    // copy attributes, the output already has had
    // the structure copied, including names and
    // number of comps.
    vtkPointData* pdSrc =
      dynamic_cast<vtkPolyData*>(this->StatsIn->GetBlock(blockId))->GetPointData();
    vtkPointData* pdDest = statsPd->GetPointData();
    const int nArrays = pdSrc->GetNumberOfArrays();
    for (int arrayId = 0; arrayId < nArrays; ++arrayId)
    {
      vtkDataArray* srcDa = pdSrc->GetArray(arrayId);
      vtkDataArray* destDa = pdDest->GetArray(arrayId);
      destDa->SetNumberOfTuples(nCenters);
      for (int i = 0; i < nCenters; ++i)
      {
        int fragmentId = this->IntersectionIds[blockId][i];
        destDa->SetTuple(i, srcDa->GetTuple(fragmentId));
      }
    }
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkIntersectFragments::CleanUpAfterRequest()
{
  this->FragmentIds.clear();
  this->IntersectionIds.clear();
  ClearVectorOfVtkPointers(this->IntersectionCenters);
  this->NFragmentsIntersected.clear();
  this->GeomIn = 0;
  this->GeomOut = 0;
  this->StatsIn = 0;
  this->StatsOut = 0;
  this->NBlocks = 0;
  return 1;
}

//----------------------------------------------------------------------------
int vtkIntersectFragments::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // Get the inputs, both are required.
  // Geometry on port 0 and statistics on port 1
  // 0
  vtkInformation* inInfo;
  inInfo = inputVector[0]->GetInformationObject(0);
  this->GeomIn = vtkMultiBlockDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (this->GeomIn == 0)
  {
    vtkErrorMacro("This filter requires a vtkMultiBlockDataSet on port 0 of its input.");
    return 1;
  }
  //   this->GeomIn=vtkMultiBlockDataSet::New();
  //   this->GeomIn->ShallowCopy(mbds);
  // 1
  inInfo = inputVector[1]->GetInformationObject(0);
  this->StatsIn = vtkMultiBlockDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (this->StatsIn == 0)
  {
    vtkErrorMacro("This filter requires a vtkMultiBlockDataSet on port 1 of its input.");
    return 1;
  }
  // Get the outputs
  // Geometry on port 0, statistic on port 1
  // 0
  vtkInformation* outInfo;
  outInfo = outputVector->GetInformationObject(0);
  this->GeomOut = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  // 1
  outInfo = outputVector->GetInformationObject(1);
  this->StatsOut = vtkMultiBlockDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Configure
  if (this->PrepareToProcessRequest() == 0)
  {
    return 0;
  }
  // Execute
  this->Intersect();
  this->UpdateProgress(0.75);
  // Compute intersection centers
  this->ComputeGeometricAttributes();
  this->UpdateProgress(0.85);
  // Gather on controller for I/O
  this->GatherGeometricAttributes(0);
  this->UpdateProgress(0.90);
  // Copy
  this->CopyAttributesToStatsOutput(0);
  this->UpdateProgress(0.99);
  // clean up
  this->CleanUpAfterRequest();

  return 1;
}

//----------------------------------------------------------------------------
// Set cut function
vtkCxxSetObjectMacro(vtkIntersectFragments, CutFunction, vtkImplicitFunction);

//----------------------------------------------------------------------------
// Overload standard modified time function. If cut function is modified,
// then this object is modified as well.
vtkMTimeType vtkIntersectFragments::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType time;

  if (this->CutFunction != NULL)
  {
    time = this->CutFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkIntersectFragments::PrintSelf(ostream& os, vtkIndent indent)
{
  // TODO print state
  this->Superclass::PrintSelf(os, indent);
}
