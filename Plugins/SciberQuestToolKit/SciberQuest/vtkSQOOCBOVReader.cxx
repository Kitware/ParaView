/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQOOCBOVReader.h"

#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkDataSetWriter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkCellData.h"
#include "vtkIntArray.h"

#include "vtkSQLog.h"
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
#include "postream.h"

#include <sstream>
#include <iostream>
#include <iomanip>

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

//#define vtkSQOOCBOVReaderDEBUG 1

#ifndef vtkSQOOCBOVReaderDEBUG
  // 0 -- no output
  // 1 -- report usage statistics
  // 2 -- adds block request trace
  // 3 -- adds block dump in vtk legacy format
  #define vtkSQOOCBOVReaderDEBUG 0
#endif

// ****************************************************************************
static
void WriteBlockUse(
      std::vector<int> &cacheHit,
      std::vector<int> &cacheMiss,
      CartesianDecomp *decomp,
      const char *fileName)
{
  vtkUnstructuredGrid *data=vtkUnstructuredGrid::New();

  vtkIntArray *hit=vtkIntArray::New();
  hit->SetName("cache-hit");
  data->GetCellData()->AddArray(hit);
  hit->Delete();

  vtkIntArray *miss=vtkIntArray::New();
  miss->SetName("cache-miss");
  data->GetCellData()->AddArray(miss);
  miss->Delete();

  int nBlocks=static_cast<int>(cacheHit.size());
  for (int q=0; q<nBlocks; ++q)
    {
    if (cacheMiss[q]>0)
      {
      *data << decomp->GetBlock(q)->GetBounds();
      *hit->WritePointer(hit->GetNumberOfTuples(),1)=cacheHit[q];
      *miss->WritePointer(miss->GetNumberOfTuples(),1)=cacheMiss[q];
      }
    }

  vtkDataSetWriter *idw=vtkDataSetWriter::New();
  idw->SetFileName(fileName);
  idw->SetInputData(data);
  idw->Write();
  idw->Delete();

  data->Delete();
}

//-----------------------------------------------------------------------------
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
  CacheMissCount(0),
  LogLevel(0)
{
  this->LRUQueue=new PriorityQueue<unsigned long int>;
}

//-----------------------------------------------------------------------------
vtkSQOOCBOVReader::~vtkSQOOCBOVReader()
{
  // this->Close(); expect the user to close
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

  int nBlocks=(int)this->DomainDecomp->GetNumberOfBlocks();

  this->LRUQueue->Initialize(this->BlockCacheSize,nBlocks);

  this->CacheHit.assign(nBlocks,0);
  this->CacheMiss.assign(nBlocks,0);
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

  int nBlocks=(int)this->DomainDecomp->GetNumberOfBlocks();
  this->CacheHit.assign(nBlocks,0);
  this->CacheMiss.assign(nBlocks,0);
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::SetCommunicator(MPI_Comm comm)
{
  this->Reader->SetCommunicator(comm);
}

//-----------------------------------------------------------------------------
int vtkSQOOCBOVReader::Open()
{
  this->ClearBlockCache();

  if (this->Image)
    {
    this->Reader->CloseTimeStep(this->Image);
    this->Image=0;
    }

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
  int worldRank=0;
  #ifndef SQTK_WITHOUT_MPI
  MPI_Comm_rank(MPI_COMM_WORLD,&worldRank);
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if ((this->LogLevel>1) || (globalLogLevel>1))
    {
    // write a datatset that could be used to visualize
    // cache hits.
    std::ostringstream oss;
    oss << "cache." << setfill('0') << setw(6) << worldRank << ".vtk";

    WriteBlockUse(
          this->CacheHit,
          this->CacheMiss,
          this->DomainDecomp,oss.str().c_str());

    log->GetBody() << worldRank << " wrote " << oss.str().c_str() << "\n";

    #ifndef SQTK_WITHOUT_MPI
    int nBlocks=(int)this->DomainDecomp->GetNumberOfBlocks();

    std::vector<int> globalCacheHit(nBlocks,0);
    MPI_Reduce(
          &this->CacheHit[0],
          &globalCacheHit[0],
          nBlocks,
          MPI_INT,
          MPI_SUM,
          0,
          MPI_COMM_WORLD);

    std::vector<int> globalCacheMiss(nBlocks,0);
    MPI_Reduce(
          &this->CacheMiss[0],
          &globalCacheMiss[0],
          nBlocks,
          MPI_INT,
          MPI_SUM,
          0,
          MPI_COMM_WORLD);

    if (worldRank==0)
      {
      oss.str("");
      oss << "cache.global.vtk";

      WriteBlockUse(
            globalCacheHit,
            globalCacheMiss,
            this->DomainDecomp,
            oss.str().c_str());

      log->GetBody() << worldRank << " wrote " << oss.str().c_str() << "\n";
      }
    #endif
    }

  if (this->LogLevel || globalLogLevel)
    {
    size_t nBlocks=this->CacheMiss.size();
    int nUsed=0;
    for (size_t i=0; i<nBlocks; ++i)
      {
      if (this->CacheMiss[i]>0)
        {
        nUsed+=1;
        }
      }

    log->GetBody()
      << worldRank
      << " vtkSQOOCBOVReader::BlockCacheStats"
      << " CacheSize=" << this->BlockCacheSize
      << " nUniqueBlocks=" << nUsed
      << " HitCount=" << this->CacheHitCount
      << " MissCount=" << this->CacheMissCount
      << "\n";
    }

  if (this->CloseClearsCachedBlocks)
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
  std::cerr << "Accessing " << Tuple<int>(block->GetId(),4);
  #endif

  // update the working domain.
  workingDomain.Set(block->GetBounds());

  // determine if the data associated with block is cached.
  vtkDataSet *data=block->GetData();
  if (data)
    {
    #if vtkSQOOCBOVReaderDEBUG>1
    std::cerr << "\tCache hit" << std::endl;
    #endif

    this->CacheHitCount+=1;
    this->CacheHit[block->GetIndex()]+=1;

    // The data is locally cached. Update the LRU queue with the block's
    // new access time, and return the cached dataset.
    this->LRUQueue->Update(block->GetIndex(),++this->BlockAccessTime);

    return data;
    }
  else
    {
    #if vtkSQOOCBOVReaderDEBUG>1
    std::cerr << "\tCache miss";
    #endif

    this->CacheMissCount+=1;
    this->CacheMiss[block->GetIndex()]+=1;

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
      std::cerr << "\tRemoved " << Tuple<int>(lruBlock->GetId(),4);
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
      std::cerr << "\tInserted " << Tuple<int>(block->GetId(),4) << std::endl;
      #endif

      // cache the newly read dataset, and insert this block into
      // the lru queue.
      block->SetData(data);
      data->Delete();
      this->LRUQueue->Push(block->GetIndex(),++this->BlockAccessTime);
      }

    #if vtkSQOOCBOVReaderDEBUG>2
    // data->Print(std::cerr);
    vtkDataSetWriter *idw=vtkDataSetWriter::New();
    std::ostringstream oss;
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
  size_t nArray=this->Reader->GetMetaData()->GetNumberOfArrays();
  for (size_t i=0; i<nArray; ++i)
    {
    const char *name=this->Reader->GetMetaData()->GetArrayName(i);
    this->Reader->GetMetaData()->DeactivateArray(name);
    }
}

//-----------------------------------------------------------------------------
void vtkSQOOCBOVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent.GetNextIndent());
  os << indent << "Reader: " << std::endl;
  this->Reader->PrintSelf(os);
  os << std::endl;
}
