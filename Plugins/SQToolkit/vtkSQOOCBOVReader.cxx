/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkSQOOCBOVReader.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkDataSetWriter.h"

#include "BOVMetaData.h"
#include "BOVReader.h"
#include "BOVTimeStepImage.h"
#include "CartesianDecomp.h"
#include "ImageDecomp.h"
#include "RectilinearDecomp.h"
#include "CartesianDataBlock.h"
#include "CartesianDataBlockIODescriptor.h"
#include "PriorityQueue.hxx"
#include "Tuple.hxx"

#include <sstream>
using std::ostringstream;

#define vtkSQOOCBOVReaderDEBUG 1

#ifndef vtkSQOOCBOVReaderDEBUG
  // 0 -- no output
  // 1 -- report usage statistics
  // 2 -- adds block request trace
  // 3 -- adds block dump in vtk legacy format
  #define vtkSQOOCBOVReaderDEBUG 0
#endif

vtkCxxRevisionMacro(vtkSQOOCBOVReader, "$Revision: 0.0 $");
vtkStandardNewMacro(vtkSQOOCBOVReader);

//-----------------------------------------------------------------------------
vtkSQOOCBOVReader::vtkSQOOCBOVReader()
      :
  Reader(0),
  Image(0),
  BlockAccessTime(0),
  BlockCacheSize(10),
  DomainDecomp(0),
  LRUQueue(0),
  CloseClearsCachedBlocks(1),
  CacheHitCount(0),
  CacheMissCount(0)
{
  this->LRUQueue=new PriorityQueue<unsigned long int>;
}

//-----------------------------------------------------------------------------
vtkSQOOCBOVReader::~vtkSQOOCBOVReader()
{
  this->Close();
  this->SetReader(0);
  this->SetDomainDecomp(0);
  delete this->LRUQueue;
}

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(vtkSQOOCBOVReader,Reader,BOVReader);

//-----------------------------------------------------------------------------
SetRefCountedPointerImpl(vtkSQOOCBOVReader,DomainDecomp,CartesianDecomp);

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::InitializeBlockCache()
{
  this->ClearBlockCache();

  int nBlocks=this->DomainDecomp->GetNumberOfBlocks();

  this->LRUQueue->Initialize(this->BlockCacheSize,nBlocks);

  this->BlockUse.assign(nBlocks,0);
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::ClearBlockCache()
{
  this->BlockAccessTime=0;

  this->CacheHitCount=0;
  this->CacheMissCount=0;

  while (!this->LRUQueue->Empty())
    {
    CartesianDataBlock *block
      = this->DomainDecomp->GetBlock(this->LRUQueue->Pop());

    block->SetData(0);
    }

  #if vtkSQOOCBOVReaderDEBUG>0
  int nBlocks=this->BlockUse.size();
  this->BlockUse.assign(nBlocks,0);
  #endif
}


//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::SetCommunicator(MPI_Comm comm)
{
  this->Reader->SetCommunicator(comm);
}

//-----------------------------------------------------------------------------
int vtkSQOOCBOVReader::Open()
{
  this->Close();

  this->Image=this->Reader->OpenTimeStep(this->TimeIndex);
  if (!this->Image)
    {
    vtkWarningMacro("Failed to open file image!");
    return 0;
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::Close()
{
  #if vtkSQOOCBOVReaderDEBUG>0
  if (this->CacheMissCount>0)
    {
    int nBlocks=this->BlockUse.size();
    int nUsed=0;
    for (int i=0; i<nBlocks; ++i)
      {
      nUsed+=this->BlockUse[i];
      }
    int worldRank=0;
    MPI_Comm_rank(MPI_COMM_WORLD,&worldRank);

    cerr
      << "[" << worldRank << "]"
      << " CacheSize=" << this->BlockCacheSize
      << " nUniqueBlocks=" << nUsed
      << " HitCount=" << this->CacheHitCount
      << " MissCount=" << this->CacheMissCount
      << endl;
    }
  #endif

  if (CloseClearsCachedBlocks)
    {
    this->ClearBlockCache();
    }

  if (this->Image)
    {
    this->Reader->CloseTimeStep(this->Image);
    this->Image=0;
    }
}

//-----------------------------------------------------------------------------
vtkDataSet *vtkSQOOCBOVReader::ReadNeighborhood(
    const double pt[3],
    CartesianBounds &workingDomain)
{

  // Locate the block containing the given point.
  CartesianDataBlock *block=this->DomainDecomp->GetBlock(pt);
  if (block==0)
    {
    vtkErrorMacro(
        << "No block in "
        << this->DomainDecomp->GetBounds()
        << " contains "
        << Tuple<double>(pt,3)
        << ".");
    return 0;
    }

  #if vtkSQOOCBOVReaderDEBUG>1
  cerr << "Accessing " << Tuple<int>(block->GetId(),4);
  #endif

  // update the working domain.
  workingDomain.Set(block->GetBounds());

  // determine if the data associated with block is cached.
  vtkDataSet *data=block->GetData();
  if (data)
    {
    #if vtkSQOOCBOVReaderDEBUG>1
    cerr << "\tCache hit" << endl;
    #endif

    #if vtkSQOOCBOVReaderDEBUG>0
    ++this->CacheHitCount;
    #endif

    // The data is locally cached. Update the LRU queue with the block's
    // new access time, and return the cached dataset.
    this->LRUQueue->Update(block->GetIndex(),++this->BlockAccessTime);

    return data;
    }
  else
    {
    #if vtkSQOOCBOVReaderDEBUG>1
    cerr << "\tCache miss";
    #endif

    #if vtkSQOOCBOVReaderDEBUG>0
    ++this->CacheMissCount;
    this->BlockUse[block->GetIndex()]=1;
    #endif

    // The data is not cached. If the cache is full then remove
    // the least recently used block and delete it's dataset. Insert
    // the requested block into the cache, load and return it's
    // associated dataset.
    if ((this->BlockCacheSize>0) && this->LRUQueue->Full())
      {
      // get the oldest cahched block and delete it's associated data.
      CartesianDataBlock *lruBlock
        = this->DomainDecomp->GetBlock(this->LRUQueue->Pop());

      lruBlock->SetData(0);

      #if vtkSQOOCBOVReaderDEBUG>1
      cerr << "\tRemoved " << Tuple<int>(lruBlock->GetId(),4);
      #endif
      }

    // configure a new dataset and read with ghost cells. Note: working
    // domain is smaller than the bounds of the dataset that is read.
    CartesianDataBlockIODescriptor *descr
      = this->DomainDecomp->GetBlockIODescriptor(block->GetIndex());

    const CartesianExtent &blockExt=descr->GetMemExtent();


    if (this->Reader->DataSetTypeIsImage())
      {
      ImageDecomp *idec=dynamic_cast<ImageDecomp*>(this->DomainDecomp);
      double *X0=idec->GetOrigin();
      double *dX=idec->GetSpacing();

      int nPoints[3];
      blockExt.Size(nPoints);

      double blockX0[3];
      blockExt.GetLowerBound(X0,dX,blockX0);

      vtkImageData *idata=vtkImageData::New();
      idata->SetDimensions(nPoints);
      idata->SetOrigin(blockX0);
      idata->SetSpacing(dX);

      data=idata;
      }
    else
    if (this->Reader->DataSetTypeIsRectilinear())
      {
      RectilinearDecomp *rdec=dynamic_cast<RectilinearDecomp*>(this->DomainDecomp);

      int nPoints[3];
      blockExt.Size(nPoints);

      vtkRectilinearGrid *rdata=vtkRectilinearGrid::New();
      rdata->SetExtent(const_cast<int*>(blockExt.GetData()));

      vtkFloatArray *fa;
      fa=vtkFloatArray::New();
      fa->SetArray(rdec->SubsetCoordinate(0,blockExt),nPoints[0],0);
      rdata->SetXCoordinates(fa);
      fa->Delete();

      fa=vtkFloatArray::New();
      fa->SetArray(rdec->SubsetCoordinate(1,blockExt),nPoints[1],0);
      rdata->SetYCoordinates(fa);
      fa->Delete();

      fa=vtkFloatArray::New();
      fa->SetArray(rdec->SubsetCoordinate(2,blockExt),nPoints[2],0);
      rdata->SetZCoordinates(fa);
      fa->Delete();

      data=rdata;
      }
    else
    if (this->Reader->DataSetTypeIsStructured())
      {
      vtkErrorMacro("Path for vtkSturcturedData not implemented.");
      return 0;
      }
    else
      {
      vtkErrorMacro("Unsupported dataset type \"" << this->Reader->GetDataSetType() << "\".");
      return 0;
      }

    int ok=this->Reader->ReadTimeStep(this->Image,descr,data,(vtkAlgorithm*)0);
    if (!ok)
      {
      data->Delete();
      vtkErrorMacro("Read failed.");
      return 0;
      }

    // cache the dataset
    if (this->BlockCacheSize>0)
      {
      #if vtkSQOOCBOVReaderDEBUG>1
      cerr << "\tInserted " << Tuple<int>(block->GetId(),4) << endl;
      #endif

      // cache the newly read dataset, and insert this block into
      // the lru queue.
      block->SetData(data);
      data->Delete();
      this->LRUQueue->Push(block->GetIndex(),++this->BlockAccessTime);
      }

    #if vtkSQOOCBOVReaderDEBUG>2
    // data->Print(cerr);
    vtkDataSetWriter *idw=vtkDataSetWriter::New();
    ostringstream oss;
    oss << "block." << block->GetIndex() << ".vtk";
    idw->SetFileName(oss.str().c_str());
    idw->SetInput(data);
    idw->Write();
    idw->Delete();
    #endif
    }

  return data;
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::ActivateArray(const char *name)
{
  this->Reader->GetMetaData()->ActivateArray(name);
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::DeActivateArray(const char *name)
{
  this->Reader->GetMetaData()->DeactivateArray(name);
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::DeActivateAllArrays()
{
  int nArray=this->Reader->GetMetaData()->GetNumberOfArrays();
  for (int i=0; i<nArray; ++i)
    {
    const char *name=this->Reader->GetMetaData()->GetArrayName(i);
    this->Reader->GetMetaData()->DeactivateArray(name);
    }
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent.GetNextIndent());
  os << indent << "Reader: " << endl;
  this->Reader->PrintSelf(os);
  os << endl;
}
