/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCTHFragmentIntersect.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCTHFragmentIntersect.h"

#include "vtkObject.h"
#include "vtkObjectFactory.h"
// Pipeline
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
// PV interface

// Data sets
#include "vtkPolyData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkCompositeDataIterator.h"
// Arrays
#include "vtkDataObject.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkIntArray.h"
#include "vtkUnsignedIntArray.h"
#include "vtkCharArray.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
// IO
#include "vtkCTHFragmentCommBuffer.h"
// Filters
#include "vtkCutter.h"
#include "vtkImplicitFunction.h"
// STL
#include "vtksys/ios/sstream"
using vtksys_ios::ostringstream;
#include "vtkstd/vector"
using vtkstd::vector;
#include "vtkstd/string"
using vtkstd::string;
#include "vtkstd/algorithm"
// ansi c
#include <math.h>
// other
#include "vtkCTHFragmentUtils.hxx"

vtkCxxRevisionMacro(vtkCTHFragmentIntersect, "1.5");
vtkStandardNewMacro(vtkCTHFragmentIntersect);

#ifdef vtkCTHFragmentIntersectDEBUG
template<class T>
static void writeTuple(ostream &sout,T *tup, int nComp)
{
  if (nComp==1)
    {
    sout << tup[0];
    }
  else
    {
    sout << "(" << tup[0];
    for (int q=1; q<nComp; ++q)
      {
      sout << ", " << tup[q];
      }
    sout << ")";
    }
}
//
static ostream &operator<<(ostream &sout, vtkDoubleArray &da)
{
  sout << "Name:          " << da.GetName() << endl;

  int nTup = da.GetNumberOfTuples();
  int nComp = da.GetNumberOfComponents();

  sout << "NumberOfComps: " << nComp << endl;
  sout << "NumberOfTuples:" << nTup << endl;
  if (nTup==0)
    {
    sout << "{}" << endl;
    }
  else
    {
    sout << "{";
    double *thisTup=da.GetTuple(0);
    writeTuple(sout, thisTup, nComp);
    for (int i=1; i<nTup; ++i)
      {
      thisTup=da.GetTuple(i);
      sout << ", ";
      writeTuple(sout, thisTup, nComp);
      }
    sout << "}" << endl;
    }
  return sout;
}
//
static ostream &operator<<(ostream &sout, vector<vtkDoubleArray *> &vda)
{
  int nda=vda.size();
  for (int i=0; i<nda; ++i)
    {
    sout << "[" << i << "]\n" << *vda[i] << endl;
    }
  return sout;
}
//
static ostream &operator<<(ostream &sout, vector<vector<int> >&vvi)
{
  int nv=vvi.size();
  for (int i=0; i<nv; ++i)
    {
    sout << "[" << i << "]{";
    int ni=vvi[i].size();
    if (ni<1) 
      {
      sout << "}" << endl;
      continue;
      }
    sout << vvi[i][0];
    for (int j=1; j<ni; ++j)
      {
      sout << vvi[i][j] << ",";
      }
    sout << "}" << endl;
    }
  return sout;
}
#endif

//----------------------------------------------------------------------------
vtkCTHFragmentIntersect::vtkCTHFragmentIntersect()
{
  this->SetNumberOfInputPorts(2);
  this->SetNumberOfOutputPorts(2);

  this->Controller
    = vtkMultiProcessController::GetGlobalController();

  this->Cutter=vtkCutter::New();
  this->CutFunction=0;

  this->GeomIn=0;
  this->GeomOut=0;
  this->StatsIn=0;
  this->StatsOut=0;
  this->NBlocks=0;

  this->Progress=0.0;
}

//----------------------------------------------------------------------------
vtkCTHFragmentIntersect::~vtkCTHFragmentIntersect()
{
  this->Controller=0;
  ClearVectorOfVtkPointers(this->IntersectionCenters);
  CheckAndReleaseVtkPointer(this->Cutter);
  this->SetCutFunction(0);
}

//----------------------------------------------------------------------------
// Input:
// 0 is expected to be distributed fragment geometry
// 1 is expected to be fragment statistics on proc 0
int vtkCTHFragmentIntersect::FillInputPortInformation(int /*port*/,
                                                    vtkInformation *info)
{
  // both inputs are mulitblock
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");

  return 1;
}

//----------------------------------------------------------------------------
// Output:
// 0 intersection statistics on proc 0
// 1 intersection geometry, distributed
int vtkCTHFragmentIntersect::FillOutputPortInformation(int port, vtkInformation *info)
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
      assert( 0 && "Invalid output port." );
      break;
    }

  return 1;
}

//----------------------------------------------------------------------------
// Connect pipeline
void vtkCTHFragmentIntersect::SetGeometryInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(0, algOutput);
}

//----------------------------------------------------------------------------
// Connect pipeline
void vtkCTHFragmentIntersect::SetStatisticsInputConnection(
                vtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Make a destination data set with the same structure as the
// source. The sources are expected to be multi block of either
// polydata or multipiece of polydata.
//
// return 0 if we couldn't copy the structure.
int vtkCTHFragmentIntersect::CopyInputStructureStats(
                vtkMultiBlockDataSet *dest,
                vtkMultiBlockDataSet *src)
{
  assert("Unexpected number of blocks in the statistics input."
         && this->NBlocks==src->GetNumberOfBlocks() );

  dest->SetNumberOfBlocks(this->NBlocks);

  // do we have an empty data set??
  if (this->NBlocks==0)
    {
    return 0;
    }
  // copy point data structure, to get the names
  // and numbers of components.
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    vtkPolyData *srcPd
      = dynamic_cast<vtkPolyData *>(src->GetBlock(blockId));
    // stats exists on only one proc if we find empty block
    // assume this is not it. Its not an error.
    if (srcPd==0)
      {
      break;
      }
    vtkPolyData *destPd=vtkPolyData::New();
    destPd->GetPointData()->CopyStructure(srcPd->GetPointData());
    dest->SetBlock(blockId,destPd);
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
int vtkCTHFragmentIntersect::CopyInputStructureGeom(
                vtkMultiBlockDataSet *dest,
                vtkMultiBlockDataSet *src)
{
  dest->SetNumberOfBlocks(this->NBlocks);

  // do we have an empty data set??
  if (this->NBlocks==0)
    {
    return 0;
    }

  // for non-empty data sets we expect that all
  // blocks are multipiece of polydata (geom)
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    vtkMultiPieceDataSet *srcFragments
      = dynamic_cast<vtkMultiPieceDataSet *>(src->GetBlock(blockId));

    if (srcFragments!=0)
      {
      vtkMultiPieceDataSet *destFragments=vtkMultiPieceDataSet::New();
      int nSrcFragments=srcFragments->GetNumberOfPieces();
      destFragments->SetNumberOfPieces(nSrcFragments);
      dest->SetBlock(blockId, destFragments);
      destFragments->Delete();

      #ifdef vtkCTHFragmentIntersectDEBUG
      cerr << "[" << __LINE__ << "]"
           << "[" << this->Controller->GetLocalProcessId() << "]"
           << "Input block "
           << blockId
           << " has "
           << nSrcFragments
           << " fragments."
           << endl;
      #endif
      }
    else
      {
      assert( "Unexpected input structure." 
              && blockId==0 );
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
int vtkCTHFragmentIntersect::IdentifyLocalFragments()
{
  int nProcs=this->Controller->GetNumberOfProcesses();
  this->FragmentIds.clear();
  this->FragmentIds.resize(this->NBlocks);
  // look in each material 
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    vtkMultiPieceDataSet *fragments
      = dynamic_cast<vtkMultiPieceDataSet *>(this->GeomIn->GetBlock(blockId));
    assert("Could not get fragments." && fragments );
    int nFragments=fragments->GetNumberOfPieces();
    this->FragmentIds[blockId].reserve(nFragments/nProcs);
    // at each fragment
    for (int fragmentId=0; fragmentId<nFragments; ++fragmentId)
      {
      vtkPolyData *fragment
        = dynamic_cast<vtkPolyData *>(fragments->GetPiece(fragmentId));
      if (fragment!=0)
        {
        // it's here, make a note.
        this->FragmentIds[blockId].push_back(fragmentId);
        }
      }
      // free extra memory
      vector<int>(this->FragmentIds[blockId]).swap(this->FragmentIds[blockId]);
    }

  #ifdef vtkCTHFragmentIntersectDEBUG
  cerr << "[" << __LINE__ << "]" 
       << "[" << this->Controller->GetLocalProcessId() << "]"
       << "found local ids:"
       << this->FragmentIds;
  #endif

  return 1;
}

//----------------------------------------------------------------------------
// Probe the input, configure internals and output accordingly
//
// return 0 on error.
int vtkCTHFragmentIntersect::PrepareToProcessRequest()
{
  // containers hold arrays for each block
  this->NBlocks=this->GeomIn->GetNumberOfBlocks();
  // size containers
  ResizeVectorOfVtkArrayPointers(
    this->IntersectionCenters,3,0,"centers",this->NBlocks);
  this->IntersectionIds.resize(this->NBlocks);
  // prepare the output data sets
  if ( (this->CopyInputStructureGeom(this->GeomOut,this->GeomIn)==0)
       || (this->CopyInputStructureStats(this->StatsOut,this->StatsIn)==0) )
    {
    vtkErrorMacro("Unexpected input structure.");
    return 0;
    }
  // Find out who we have with us.
  this->IdentifyLocalFragments();
  // configure the cutter
  this->Cutter->SetCutFunction(this->CutFunction);
  // other
  this->Progress=0.0;
  this->ProgressIncrement=0.75/(double)this->NBlocks;

  return 1;
}

//----------------------------------------------------------------------------
// Take the intersection of all fragments we own with the 
// specified implicit function.
//
// return 0 on error
int vtkCTHFragmentIntersect::Intersect()
{
  // look in each material
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    vtkDoubleArray *centers=this->IntersectionCenters[blockId];
    vector<int> &ids=this->IntersectionIds[blockId];
    vtkMultiPieceDataSet *geomOut
      = dynamic_cast<vtkMultiPieceDataSet *>(this->GeomOut->GetBlock(blockId));
    // for fragments we own
    vtkMultiPieceDataSet *fragments
      = dynamic_cast<vtkMultiPieceDataSet *>(this->GeomIn->GetBlock(blockId));
    vector<int> &fragmentIds=this->FragmentIds[blockId];
    int nLocal=fragmentIds.size();
    for (int i=0; i<nLocal; ++i)
      {
      int fragmentId=fragmentIds[i];
      vtkPolyData *fragment
        = dynamic_cast<vtkPolyData *>(fragments->GetPiece(fragmentId));
      // cut
      this->Cutter->SetInput(fragment);
      vtkPolyData *intersection=this->Cutter->GetOutput();
      intersection->Update();
      if (intersection->GetNumberOfPoints()>0)
        {
        // calculate intersection center
        double aabb[6];
        intersection->GetBounds(aabb);
        double cen[3];
        for (int q=0; q<3; ++q)
          {
          cen[q]=(aabb[2*q]+aabb[2*q+1])/2.0;
          }
        centers->InsertNextTuple(cen);
        ids.push_back(fragmentId);
        // pass intersection geometry
        vtkPolyData *intersectionOut=vtkPolyData::New();
        intersectionOut->ShallowCopy(intersection);
        geomOut->SetPiece(i,intersectionOut);
        intersectionOut->Delete();
        }
      }
    // free extra memory
    centers->Squeeze();
    vector<int>(ids).swap(ids);

    this->Progress+=this->ProgressIncrement;
    this->UpdateProgress(this->Progress);
    }

  #ifdef vtkCTHFragmentIntersectDEBUG
  cerr << "[" << __LINE__ << "]"
       << "[" << this->Controller->GetLocalProcessId() << "]"
       << "intersection produced:"
       << endl
       << this->IntersectionCenters
       << this->IntersectionIds;
  #endif
  return 1;
}

//----------------------------------------------------------------------------
// Receive all geomteric attribute arrays from all other
// processes. Conatiners filled with the expected number
// of empty data arrays/pointers are expected.
// 
// The following structure is expected:
// attribute[nProcs][nMaterials]
// buffers[nProcs]
//
// return 0 on error
int vtkCTHFragmentIntersect::CollectGeometricAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vector<vtkDoubleArray *> > &centers,
                vector<vector<int *> > &ids)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();
  const int msgBase=200000;

  // size header.
  vtkCTHFragmentCommBuffer::SizeHeader(buffers,this->NBlocks);

  // gather
  for (int procId=0; procId<nProcs; ++procId)
    {
    // skip mine
    if (procId==myProcId)
      {
      continue;
      }
    int thisMsgId=msgBase;
    // receive header
    this->Controller->Receive(
            buffers[procId].GetHeader(),
            buffers[procId].GetHeaderSize(),
            procId,
            thisMsgId);
    ++thisMsgId;
    // size buffer (recvd its size in header)
    buffers[procId].SizeBuffer();
    // receive buffer
    this->Controller->Receive(
            buffers[procId].GetBuffer(),
            buffers[procId].GetBufferSize(),
            procId,
            thisMsgId);
    ++thisMsgId;
    // unpack buffer
    for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
      {
      int nFragments
        = buffers[procId].GetNumberOfTuples(blockId);
      // centers, memory managed by comm buffer.
      buffers[procId].UnPack(centers[procId][blockId],3,nFragments,false);
      // ids, memory managed by comm buffer.
      buffers[procId].UnPack(ids[procId][blockId],1,nFragments,false);
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
// Send all geometric attributes for the fragments which I own
// to another process.
//
// return 0 on error
int vtkCTHFragmentIntersect::SendGeometricAttributes(const int recipientProcId)
{
  const int msgBase=200000;

  const int nCompsPerBlock=3; // centers
  vector<int> nFragments(this->NBlocks);

  // size buffer & initialize header
  vtkCTHFragmentCommBuffer buffer;
  buffer.SizeHeader(this->NBlocks);
  int nBytes=0;
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    nFragments[blockId]=this->IntersectionIds[blockId].size();
    nBytes  // attributes(double) + ids(int)
      += nFragments[blockId]*(nCompsPerBlock*sizeof(double)+sizeof(int));
    buffer.SetNumberOfTuples(blockId,nFragments[blockId]);
    }
  buffer.SizeBuffer(nBytes);

  // pack attributes & ids
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    // centers
    buffer.Pack(this->IntersectionCenters[blockId]);
    // ids
    buffer.Pack(&this->IntersectionIds[blockId][0],1,nFragments[blockId]);
    }

  // send 
  int thisMsgId=msgBase;
  // header
  this->Controller->Send(
          buffer.GetHeader(),
          buffer.GetHeaderSize(),
          recipientProcId,
          thisMsgId);
  ++thisMsgId;
  // buffer
  this->Controller->Send(
          buffer.GetBuffer(),
          buffer.GetBufferSize(),
          recipientProcId,
          thisMsgId);
  ++thisMsgId;

  return 1;
}

//------------------------------------------------------------------------------
// Configure buffers and containers, and put our data in.
// attribute[nProc][nMaterial]->array
//
// return 0 on error
int vtkCTHFragmentIntersect::PrepareToCollectGeometricAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vector<vtkDoubleArray *> > &centers,
                vector<vector<int *> > &ids)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // buffers
  buffers.resize(nProcs);
  // centers
  centers.resize(nProcs);
  for (int procId=0; procId<nProcs; ++procId)
    {
    if (myProcId==procId)
      {
      centers[procId]=this->IntersectionCenters;
      }
    else
      {
      ResizeVectorOfVtkPointers(centers[procId],this->NBlocks);
      }
    }
  // ids
  ids.resize(nProcs);
  for (int procId=0; procId<nProcs; ++procId)
    {
    ids[procId].resize(this->NBlocks,static_cast<int *>(0));
    //
    if (procId==myProcId)
      {
      for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
        {
        // Because we are going to us IntersectionIds to 
        // hold the merged ids we copy our ids to a temp array.
        int nCenters=this->IntersectionIds[blockId].size();
        ids[myProcId][blockId]=new int[nCenters];
        for (int i=0; i<nCenters; ++i)
          {
          ids[myProcId][blockId][i]=this->IntersectionIds[blockId][i];
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
int vtkCTHFragmentIntersect::CleanUpAfterCollectGeometricAttributes(
                vector<vtkCTHFragmentCommBuffer> &buffers,
                vector<vector<vtkDoubleArray *> >&centers,
                vector<vector<int *> >&ids)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // centers
  for (int procId=0; procId<nProcs; ++procId)
    {
    ClearVectorOfVtkPointers(centers[procId]);
    }
  // ids
  // clean up local temp array. Remote procs are
  // using the comm buffers, they are managed there.
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    delete [] ids[myProcId][blockId];
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
int vtkCTHFragmentIntersect::PrepareToMergeGeometricAttributes(
                vector<vector<vtkDoubleArray *> > &centers)
{
  const int nProcs=this->Controller->GetNumberOfProcesses();

  // compute size gathered arrays
  vector<int> nCenters(this->NBlocks,0);
  for (int procId=0; procId<nProcs; ++procId)
    {
    for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
      {
      nCenters[blockId]
        += centers[procId][blockId]->GetNumberOfTuples();
      }
    }
  // size gathered arrays
  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    //centers
    NewVtkArrayPointer(
        this->IntersectionCenters[blockId],
        3,
        nCenters[blockId],
        this->IntersectionCenters[blockId]->GetName());
    // ids
    this->IntersectionIds[blockId].resize(nCenters[blockId]);
    }
  return 1;
}
//----------------------------------------------------------------------------
// Gather all geometric attributes to a single process.
// 
// return 0 on error
int vtkCTHFragmentIntersect::GatherGeometricAttributes(
                const int recipientProcId)
{
  const int myProcId=this->Controller->GetLocalProcessId();
  const int nProcs=this->Controller->GetNumberOfProcesses();

  if (myProcId==recipientProcId)
    {
    // collect
    vector<vtkCTHFragmentCommBuffer>buffers;
    vector<vector<vtkDoubleArray *> >centers;
    vector<vector<int *> >ids;
    this->PrepareToCollectGeometricAttributes(buffers,centers,ids);
    this->CollectGeometricAttributes(buffers,centers,ids);
    // merge
    this->PrepareToMergeGeometricAttributes(centers);
    vector<int> mergedIdx(this->NBlocks,0);// counts merged so far
    for (int procId=0; procId<nProcs; ++procId)
      {
      for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
        {
        const int idx=mergedIdx[blockId];
        const int nToMerge
          = centers[procId][blockId]->GetNumberOfTuples();

        // locate
        // centers
        const int nCenterComps=3;
        const double *pRemoteCenters=centers[procId][blockId]->GetPointer(0);
        double *pLocalCenters
          = this->IntersectionCenters[blockId]->GetPointer(idx*nCenterComps);
        // ids
        const int *pRemoteIds=ids[procId][blockId];

        // copy fromm remote into local/merged
        for (int i=0; i<nToMerge; ++i)
          {
          // centers
          for (int q=0; q<nCenterComps; ++q)
            {
            pLocalCenters[q]=pRemoteCenters[q];
            }
          pLocalCenters+=nCenterComps;
          pRemoteCenters+=nCenterComps;
          // ids
          this->IntersectionIds[blockId][idx+i]=pRemoteIds[0];
          ++pRemoteIds;
          }
        mergedIdx[blockId]+=nToMerge;
        }
      }
    this->CleanUpAfterCollectGeometricAttributes(buffers,centers,ids);
    }
  else
    {
    this->SendGeometricAttributes(recipientProcId);
    }

  return 1;
}
//----------------------------------------------------------------------------
int vtkCTHFragmentIntersect::CopyAttributesToStatsOutput(
  const int controllingProcId)
{
  const int myProcId=this->Controller->GetLocalProcessId();

  if (myProcId!=controllingProcId)
    {
    // anything else??
    return 1;
    }

  #ifdef vtkCTHFragmentIntersectDEBUG
  cerr << "[" << __LINE__ << "]"
       << "[" << myProcId << "]"
       << "copying to stats output:"
       << this->IntersectionCenters
       << this->IntersectionIds;
  #endif

  for (unsigned int blockId=0; blockId<this->NBlocks; ++blockId)
    {
    vtkPolyData *statsPd
      = dynamic_cast<vtkPolyData *>(this->StatsOut->GetBlock(blockId));

    const vtkIdType nCenters
      = this->IntersectionCenters[blockId]->GetNumberOfTuples();
    // add points and vertices.
    vtkIdTypeArray *va=vtkIdTypeArray::New();
    va->SetNumberOfTuples(2*nCenters);
    vtkIdType *verts=va->GetPointer(0);
    vtkPoints *pts=vtkPoints::New();
    pts->SetData(this->IntersectionCenters[blockId]);
    for (int i=0; i<nCenters; ++i)
      {
      verts[0]=1;
      verts[1]=i;
      verts+=2;
      }
    statsPd->SetPoints(pts);
    pts->Delete();
    vtkCellArray *cells=vtkCellArray::New();
    cells->SetCells(nCenters,va);
    statsPd->SetVerts(cells);
    cells->Delete();
    va->Delete();
    // copy attributes, the output already has had
    // the structure coppied, including names and
    // number of comps.
    vtkPointData *pdSrc
      = dynamic_cast<vtkPolyData *>(this->StatsIn->GetBlock(blockId))->GetPointData();
    vtkPointData *pdDest=statsPd->GetPointData();
    const int nArrays=pdSrc->GetNumberOfArrays();
    for (int arrayId=0; arrayId<nArrays; ++arrayId)
      {
      vtkDataArray *srcDa=pdSrc->GetArray(arrayId);
      vtkDataArray *destDa=pdDest->GetArray(arrayId);
      destDa->SetNumberOfTuples(nCenters);
      for (int i=0; i<nCenters; ++i)
        {
        int fragmentId=this->IntersectionIds[blockId][i];
        destDa->SetTuple(i,srcDa->GetTuple(fragmentId));
        }
      }
    }

  return 1;
}
//----------------------------------------------------------------------------
int vtkCTHFragmentIntersect::CleanUpAfterRequest()
{
  this->FragmentIds.clear();
  this->IntersectionIds.clear();
  ClearVectorOfVtkPointers(this->IntersectionCenters);
  this->GeomIn=0;
  this->GeomOut=0;
  this->StatsIn=0;
  this->StatsOut=0;
  this->NBlocks=0;
  return 1;
}

//----------------------------------------------------------------------------
int vtkCTHFragmentIntersect::RequestData(
                vtkInformation *vtkNotUsed(request),
                vtkInformationVector **inputVector,
                vtkInformationVector *outputVector)
{
  // Get the inputs, both are required.
  // Geometry on port 0 and statistics on port 1
  // 0
  vtkInformation *inInfo;
  inInfo = inputVector[0]->GetInformationObject(0);
  this->GeomIn=vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( this->GeomIn==0 )
    {
    vtkErrorMacro("This filter requires a vtkMultiBlockDataSet on port 0 of its input.");
    return 1;
    }
//   this->GeomIn=vtkMultiBlockDataSet::New();
//   this->GeomIn->ShallowCopy(mbds);
  // 1
  inInfo = inputVector[1]->GetInformationObject(0);
  this->StatsIn=vtkMultiBlockDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if ( this->StatsIn==0 )
    {
    vtkErrorMacro("This filter requires a vtkMultiBlockDataSet on port 1 of its input.");
    return 1;
    }
  // Get the outputs
  // Geometry on port 0, statistic on port 1
  // 0
  vtkInformation *outInfo;
  outInfo = outputVector->GetInformationObject(0);
  this->GeomOut =
    vtkMultiBlockDataSet::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );
  // 1
  outInfo = outputVector->GetInformationObject(1);
  this->StatsOut =
    vtkMultiBlockDataSet::SafeDownCast( outInfo->Get(vtkDataObject::DATA_OBJECT()) );

  // Configure
  if (this->PrepareToProcessRequest()==0)
    {
    return 0;
    }
  // Execute
  this->Intersect();
  this->UpdateProgress(0.75);
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
vtkCxxSetObjectMacro(vtkCTHFragmentIntersect,CutFunction,vtkImplicitFunction);


//----------------------------------------------------------------------------
// Overload standard modified time function. If cut function is modified,
// then this object is modified as well.
unsigned long vtkCTHFragmentIntersect::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if ( this->CutFunction != NULL )
    {
    time = this->CutFunction->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkCTHFragmentIntersect::PrintSelf(ostream& os, vtkIndent indent)
{
  // TODO print state
  this->Superclass::PrintSelf(os,indent);
}
