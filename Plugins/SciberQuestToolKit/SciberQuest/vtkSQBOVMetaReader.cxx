/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBOVMetaReader.h"
#include <vtksys/SystemInformation.hxx>
#include "vtkObjectFactory.h"
#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkPVXMLElement.h"
#include "vtkPVInformationKeys.h"

#include "vtkSQLog.h"
#include "vtkSQOOCReader.h"
#include "vtkSQOOCBOVReader.h"
#include "BOVReader.h"
#include "GDAMetaData.h"
#include "BOVTimeStepImage.h"
#include "ImageDecomp.h"
#include "RectilinearDecomp.h"
#include "Numerics.hxx"
#include "Tuple.hxx"
#include "XMLUtils.h"
#include "PrintUtils.h"
#include "SQMacros.h"
#include "postream.h"

#ifndef SQTK_WITHOUT_MPI
#include "SQMPICHWarningSupression.h"
#include <mpi.h>
#endif

#include <algorithm>
#include <sstream>

// #define SQTK_DEBUG

#ifdef WIN32
  #undef SQTK_DEBUG
#endif

// ****************************************************************************
static
unsigned long hash(const unsigned char *str)
{
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQBOVMetaReader);

//-----------------------------------------------------------------------------
vtkSQBOVMetaReader::vtkSQBOVMetaReader()
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss << "=====vtkSQBOVMetaReader::vtkSQBOVMetaReader" << std::endl;
  #endif

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Initialize variables
  this->UseCollectiveIO=HINT_DISABLED;
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->NGhosts=1;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->BlockSize[0]=
  this->BlockSize[1]=
  this->BlockSize[2]=96;
  this->BlockCacheRamFactor=0.75;
  this->ProcRam=0;

  #if defined SQTK_DEBUG
  pCerr() << oss.str() << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
vtkSQBOVMetaReader::~vtkSQBOVMetaReader()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVMetaReader::~vtkSQBOVMetaReader" << std::endl;
  #endif

  this->Clear();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::Clear()
{
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->NGhosts=1;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->BlockSize[0]=
  this->BlockSize[1]=
  this->BlockSize[2]=96;
  this->BlockCacheRamFactor=0.75;
  this->ProcRam=0;

  vtkSQBOVReaderBase::Clear();
}

//-----------------------------------------------------------------------------
int vtkSQBOVMetaReader::Initialize(
      vtkPVXMLElement *root,
      const char *fileName,
      std::vector<std::string> &arrays)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=GetOptionalElement(root,"vtkSQBOVMetaReader");
  if (elem==0)
    {
    return -1;
    }

  // initialize the base class here as when the file is
  // opened the block cache settings are reset.
  if (vtkSQBOVReaderBase::Initialize(root,fileName,arrays))
    {
    return -1;
    }

  // this will initialize the block cache.
  int block_size[3]={96,96,96};
  GetOptionalAttribute<int,3>(elem,"block_size",block_size);
  this->SetBlockSize(block_size);

  // TODO -- See comment in EstimateBlockCacheSize
  /*
  double block_cache_ram_factor=0.75;
  GetOptionalAttribute<double,1>(elem,"block_cache_ram_factor",&block_cache_ram_factor);
  this->SetBlockCacheRamFactor(block_cache_ram_factor);
  */

  // if these are provided by the user, then this will
  // override the default intialization of the block cache
  int decomp_dims[3]={0,0,0};
  GetOptionalAttribute<int,3>(elem,"decomp_dims",decomp_dims);
  if (decomp_dims[0]>0)
    {
    this->SetDecompDims(decomp_dims);
    }

  int block_cache_size=0;
  GetOptionalAttribute<int,1>(elem,"block_cache_size",&block_cache_size);
  if (block_cache_size>0)
    {
    this->SetBlockCacheSize(block_cache_size);
    }

  int periodic_bc[3]={0,0,0};
  GetOptionalAttribute<int,3>(elem,"periodic_bc",periodic_bc);
  this->SetPeriodicBC(periodic_bc);

  int n_ghosts=1;
  GetOptionalAttribute<int,1>(elem,"n_ghosts",&n_ghosts);
  if (n_ghosts>1)
    {
    this->SetNumberOfGhostCells(n_ghosts);
    }

  int clear_cache=1;
  GetOptionalAttribute<int,1>(elem,"clear_cache",&clear_cache);
  if (clear_cache==0)
    {
    this->SetClearCachedBlocks(0);
    }

  this->SetUseCollectiveIO(vtkSQBOVMetaReader::HINT_DISABLED);

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQBOVMetaReader" << "\n"
      << "#   block_size=" << Tuple<int>(this->BlockSize,3) << "\n"
      << "#   block_cache_ram_factor=" << this->BlockCacheRamFactor << "\n"
      << "#   decomp_dims=" << Tuple<int>(this->DecompDims,3) << "\n"
      << "#   block_cache_size=" << this->BlockCacheSize << "\n"
      << "#   periodic_bc=" << Tuple<int>(this->PeriodicBC,3) << "\n"
      << "#   n_ghosts=" << this->NGhosts << "\n"
      << "#   clear_cache=" << this->ClearCachedBlocks << "\n";
    }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetFileName(const char* _arg)
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss
    << "=====vtkSQBOVReader::SetFileName" << std::endl
    << (_arg==NULL?"NULL":_arg) << std::endl;
  #endif

  if (this->FileName == NULL && _arg == NULL)
    {
    return;
    }
  if (this->FileName && _arg && (!strcmp(this->FileName,_arg)))
    {
    return;
    }

  this->vtkSQBOVReaderBase::SetFileName(_arg);

  if (_arg == NULL)
    {
    return;
    }

  BOVMetaData *md=this->Reader->GetMetaData();
  if (md->IsDatasetOpen())
    {
    // if the file name changes the block cache parameters
    // should be re-set. the down side of doing it here is
    // that it if users sets the decomp dims first they are
    // overrode
    this->EstimateBlockCacheSize();
    }
}

//-----------------------------------------------------------------------------
long long vtkSQBOVMetaReader::GetProcRam()
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss << "=====vtkSQBOVReader::GetProcRam" << std::endl;
  #endif
  if (this->ProcRam==0)
    {
    // gather memory info about this host
    vtksys::SystemInformation sysInfo;

    long long hostRam=sysInfo.GetHostMemoryAvailable(
            "PV_HOST_MEMORY_LIMIT");

    long long procRam=sysInfo.GetProcMemoryAvailable(
            "PV_HOST_MEMORY_LIMIT",
            "PV_PROC_MEMORY_LIMIT");

    std::string hostName=sysInfo.GetHostname();
    unsigned long hostId=hash((const unsigned char *)hostName.c_str());
    long long hostSize=1l;

    #ifndef SQTK_WITHOUT_MPI
    int worldSize=1;
    MPI_Comm_size(MPI_COMM_WORLD,&worldSize);

    std::vector<unsigned long> hostIds(worldSize,0ul);
    MPI_Allgather(
          &hostId,
          1,
          MPI_UNSIGNED_LONG,
          &hostIds[0],
          1,
          MPI_UNSIGNED_LONG,
          MPI_COMM_WORLD);

    hostSize=(long long)count(hostIds.begin(),hostIds.end(),hostId);
    #endif

    this->ProcRam=std::min(procRam,hostRam/hostSize);

    #if defined SQTK_DEBUG
    oss
      << "hostName=" << hostName << std::endl
      << "hostId=" << hostId << std::endl
      << "procRam=" << procRam << std::endl
      << "hostRam=" << hostRam << std::endl
      << "hostSize=" << hostSize << std::endl
      << "ProcRam=" << this->ProcRam << std::endl;
    pCerr() << oss.str() << std::endl;
    #endif
    }

  return this->ProcRam;
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetBlockCacheRamFactor(double factor)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::SetBlockCacheRamFactor" << std::endl;
  #endif

  if (this->BlockCacheRamFactor==factor)
    {
    return;
    }

  if (factor < 0.01)
    {
    vtkErrorMacro("BlockCacheRamFactor must be greater than 0.01(1%)");
    return;
    }

  this->BlockCacheRamFactor=factor;

  BOVMetaData *md=this->Reader->GetMetaData();
  if (md->IsDatasetOpen())
    {
    this->EstimateBlockCacheSize();
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetBlockSize(int nx, int ny, int nz)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::SetBlockSize" << std::endl;
  #endif
  if ( (this->BlockSize[0]==nx)
    && (this->BlockSize[1]==ny)
    && (this->BlockSize[2]==nz) )
    {
    return;
    }

  unsigned long long blockSize=nx*ny*nz;
  if (blockSize >= 2147483648l)
    {
    vtkErrorMacro(
      << "Block size must be smaller than 2GB "
      << "because MPI uses int in its API");
    return;
    }

  this->BlockSize[0]=nx;
  this->BlockSize[1]=ny;
  this->BlockSize[2]=nz;

  BOVMetaData *md=this->Reader->GetMetaData();
  if (md->IsDatasetOpen())
    {
    CartesianExtent subset=md->GetSubset();

    if (blockSize>=subset.Size())
      {
      //vtkErrorMacro("Block size cannot be larger than the dataset itself.");
      subset.Size(this->BlockSize);
      }
    this->EstimateBlockCacheSize();
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::EstimateBlockCacheSize()
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss << "=====vtkSQBOVReader::EstimateBlockCacheSize" << std::endl;
  #endif

  BOVMetaData *md=this->Reader->GetMetaData();
  if (!md->IsDatasetOpen())
    {
    vtkErrorMacro("Dataset must be open to estimate block cache sizes.");
    return;
    }

  // compute decomp dims
  int subsetSize[3];
  md->GetSubset().Size(subsetSize);

  int decompDims[3];
  decompDims[0]=std::max(1,subsetSize[0]/this->BlockSize[0]);
  decompDims[1]=std::max(1,subsetSize[1]/this->BlockSize[1]);
  decompDims[2]=std::max(1,subsetSize[2]/this->BlockSize[2]);
  this->SetDecompDims(decompDims);

  // TODO
  // Trying to set the block cache size by the available
  // memory on the system was problematic and lead to some
  // performance issues.
  /*
  size_t blockSize=this->BlockSize[0]*this->BlockSize[1]*this->BlockSize[2];
  double blockRam=std::max(1.0,blockSize*sizeof(float)*3.0/1024.0);
  double procRam=this->GetProcRam();
  int maxBlocks=decompDims[0]*decompDims[1]*decompDims[2];
  int fitBlocks=(int)(procRam*this->BlockCacheRamFactor/blockRam);
  if (fitBlocks==0)
     {
     vtkErrorMacro(
       << "[" << this->WorldRank << "]"
       << " The selected block size " << Tuple<int>(this->BlockSize,3)
       << " does not fit in the available process ram " << procRam
       << " decrease the blocksize before continuing.");
     }
  this->SetBlockCacheSize(std::min(maxBlocks,fitBlocks));
  */

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetBody()
      << this->WorldRank
      << " vtkSQBOVMetaReader::BlockCacheSettings"
      << " BlockCacheSize=" << this->BlockCacheSize
      << " DecompDims=("
      << this->DecompDims[0] << ", "
      << this->DecompDims[1] << ", "
      << this->DecompDims[2] << ")"
      << "\n";
    }

  #if defined SQTK_DEBUG
  oss
    << "subsetSize=" << Tuple<int>(subsetSize,3) << std::endl
    << "ProcRam(kib)=" << procRam << std::endl
    << "BlockCacheRamFactor=" << this->BlockCacheRamFactor << std::endl
    << "BlockSize=" << Tuple<int>(this->BlockSize,3) << std::endl
    << "blockRam(kib)=" << blockRam << std::endl
    << "maxBlocks=" << maxBlocks << std::endl
    << "fitBlocks=" << fitBlocks << std::endl
    << "BlockCacheSize=" << this->BlockCacheSize << std::endl
    << "DecompDims=" << Tuple<int>(this->DecompDims,3) << std::endl;
  pCerr() << oss.str() << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetPeriodicBC(int *flags)
{
  if ( (this->PeriodicBC[0]==flags[0])
    && (this->PeriodicBC[1]==flags[1])
    && (this->PeriodicBC[2]==flags[2]) )
    {
    return;
    }

  this->PeriodicBC[0]=flags[0];
  this->PeriodicBC[1]=flags[1];
  this->PeriodicBC[2]=flags[2];

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetXHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[0]==flag)
    {
    return;
    }

  this->PeriodicBC[0]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetYHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[1]==flag)
    {
    return;
    }

  this->PeriodicBC[1]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::SetZHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[2]==flag)
    {
    return;
    }

  this->PeriodicBC[2]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVMetaReader::RequestInformation(
  vtkInformation *req,
  vtkInformationVector **inInfos,
  vtkInformationVector* outInfos)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVMetaReader::RequestInformation" << std::endl;
  #endif

  (void)req;
  (void)inInfos;

  if (!this->Reader->IsOpen())
    {
    vtkWarningMacro("No file open, cannot process RequestInformation!");
    return 1;
    }

  vtkInformation *info=outInfos->GetInformationObject(0);

  // In a meta read we need to trick the pipeline into
  // displaying the dataset bounds, in a single cell per
  // process. This keeps the memory footprint minimal.
  // We still need to provide the actual meta data for
  // use downstream, including a file name so that filters
  // may use it to read the actual data, we do so using
  // keys provided having the same name as the original.

  // Set the extent of the data.
  // The point extent given PV by the meta reader will always start from
  // 0 and range to nProcs and 1. This will neccesitate a false origin
  // and spacing as well.
  int wholeExtent[6]={0,this->WorldSize,0,1,0,1};

  info->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        wholeExtent,
        6);

  // req->Append(
  //     vtkExecutive::KEYS_TO_COPY(),
  //     vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  info->Set(CAN_PRODUCE_SUB_EXTENT(), 1);

  if (this->Reader->DataSetTypeIsImage())
    {
    // Adjust PV's keys for the false subsetting extents.
    double X0[3];
    this->Reader->GetMetaData()->GetOrigin(X0);

    double dX[3];
    this->Reader->GetMetaData()->GetSpacing(dX);

    X0[0]=X0[0]+this->Subset[0]*dX[0];
    X0[1]=X0[1]+this->Subset[2]*dX[1];
    X0[2]=X0[2]+this->Subset[4]*dX[2];

    // Adjust grid spacing for our single cell per process. We are using the dual
    // grid so we are subtracting 1 to get the number of cells.
    int nCells[3]={
      this->Subset[1]-this->Subset[0],
      this->Subset[3]-this->Subset[2],
      this->Subset[5]-this->Subset[4]};

    dX[0]=dX[0]*((double)nCells[0])/((double)this->WorldSize);
    dX[1]=dX[1]*((double)nCells[1]);
    dX[2]=dX[2]*((double)nCells[2]);

    // Pass values into the pipeline.
    info->Set(vtkDataObject::ORIGIN(),X0,3);
    // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::ORIGIN());

    info->Set(vtkDataObject::SPACING(),dX,3);
    // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::SPACING());
    }

  return vtkSQBOVReaderBase::RequestInformation(req,inInfos,outInfos);
}

//-----------------------------------------------------------------------------
int vtkSQBOVMetaReader::RequestData(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector *outInfos)
{
  #if defined SQTK_DEBUG
  std::ostringstream oss;
  oss << "=====vtkSQBOVMetaReader::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQBOVMetaReader::RequestData");
    }

  (void)inInfos;
  vtkInformation *info=outInfos->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *output
    = dynamic_cast<vtkDataSet *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  // get the step id of requested time.
  int stepId=this->GetTimeStepId(info,output->GetInformation());

  // The subset is the what the user selected in the GUI. This is what will
  // be loaded in aggregate across the entire run.
  BOVMetaData *md=this->Reader->GetMetaData();
  CartesianExtent subset=md->GetSubset();

  #if defined SQTK_DEBUG
  if (this->WorldRank==0)
    {
    oss
      << "subset=" << subset
      << " size=" << subset.Size()*sizeof(float)
      << std::endl;
    }
  #endif
  // shift to the dual grid
  subset.NodeToCell();

  // this is a hack to accomodate 2D grids.
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;
    if (subset[qq+1]<subset[qq])
      {
      subset[qq+1]=subset[qq];
      }
    }

  // ParaView sends the update extent to inform us of the domain decomposition.
  // The decomp is what will be loaded by this process.
  CartesianExtent decomp;
  //int decomp[6];
  info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),decomp.GetData());

  // Set the region to be read.
  md->SetDecomp(decomp);

  #if defined SQTK_DEBUG
  oss << "decomp=" << decomp << std::endl;
  #endif

  // Construct MPI File hints for the reader.
  this->SetMPIFileHints();


  // pass the boundary condition flags
  info->Set(vtkSQOOCReader::PERIODIC_BC(),this->PeriodicBC,3);
  req->Append(vtkExecutive::KEYS_TO_COPY(), vtkSQOOCReader::PERIODIC_BC());

  CartesianDecomp *ddecomp = NULL;

  // The file extents describe the data as it is on the disk.
  CartesianExtent fileExt=md->GetDomain();

  // shift to dual grid
  fileExt.NodeToCell();

  // this is a hack to accomodate 2D grids.
  for (int q=0; q<3; ++q)
    {
    int qq=2*q;
    if (fileExt[qq+1]<fileExt[qq])
      {
      fileExt[qq+1]=fileExt[qq];
      }
    }

  // This reader can read vtkImageData, vtkRectilinearGrid
  // and vtkStructuredData

  if (this->Reader->DataSetTypeIsImage())
    {
    /// Image data

    // Pull origin and spacing we stored during RequestInformation pass.
    double dX[3];
    info->Get(vtkDataObject::SPACING(),dX);
    double X0[3];
    info->Get(vtkDataObject::ORIGIN(),X0);

    // dimensions of the dummy output
    int nPoints[3];
    nPoints[0]=this->WorldSize+1;
    nPoints[1]=2;
    nPoints[2]=2;

    // Configure the output.
    vtkImageData *idds=dynamic_cast<vtkImageData*>(output);
    idds->SetDimensions(nPoints);
    idds->SetOrigin(X0);
    idds->SetSpacing(dX);
    idds->SetExtent(decomp.GetData());

    // Store the bounds of the requested subset.
    double subsetBounds[6];
    subsetBounds[0]=X0[0];
    subsetBounds[1]=X0[0]+dX[0]*((double)this->WorldSize);
    subsetBounds[2]=X0[1];
    subsetBounds[3]=X0[1]+dX[1];
    subsetBounds[4]=X0[2];
    subsetBounds[5]=X0[2]+dX[2];
    info->Set(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX());

    // Setup the user defined domain decomposition over the subset. This
    // decomposition is used to fine tune the I/O performance of out-of-core
    // filters.
    double *subsetX0=md->GetOrigin();
    double *subsetDX=md->GetSpacing();

    ImageDecomp *iddecomp=ImageDecomp::New();
    iddecomp->SetFileExtent(fileExt);
    iddecomp->SetExtent(subset);
    iddecomp->SetOrigin(subsetX0);
    iddecomp->SetSpacing(subsetDX);
    iddecomp->SetNumberOfGhostCells(this->NGhosts);
    iddecomp->ComputeDimensionMode();
    iddecomp->ComputeBounds();
    iddecomp->SetDecompDims(this->DecompDims);
    iddecomp->SetPeriodicBC(this->PeriodicBC);
    int ok=iddecomp->DecomposeDomain();
    if (!ok)
      {
      vtkErrorMacro("Failed to decompose domain.");
      output->Initialize();
      return 1;
      }
    ddecomp=iddecomp;
    }
  else
  if (this->Reader->DataSetTypeIsRectilinear())
    {
    /// Rectilinear grid
    // Store the bounds of the requested subset.
    double subsetBounds[6]={
      md->GetCoordinate(0)->GetPointer()[subset[0]],
      md->GetCoordinate(0)->GetPointer()[subset[1]+1],
      md->GetCoordinate(1)->GetPointer()[subset[2]],
      md->GetCoordinate(1)->GetPointer()[subset[3]+1],
      md->GetCoordinate(2)->GetPointer()[subset[4]],
      md->GetCoordinate(2)->GetPointer()[subset[5]+1]};
    info->Set(vtkPVInformationKeys::WHOLE_BOUNDING_BOX(),subsetBounds,6);
    req->Append(
        vtkExecutive::KEYS_TO_COPY(),
        vtkPVInformationKeys::WHOLE_BOUNDING_BOX());

    // Store the bounds of the requested subset.
    int nCells[3];
    subset.Size(nCells);

    int nLocal=nCells[0]/this->WorldSize;
    int nLarge=nCells[0]%this->WorldSize;

    int ilo;
    int ihi;

    if (this->WorldRank<nLarge)
      {
      ilo=subset[0]+this->WorldRank*(nLocal+1);
      ihi=ilo+nLocal+1;
      }
    else
      {
      ilo=subset[0]+this->WorldRank*nLocal+nLarge;
      ihi=ilo+nLocal;
      }

    // Configure the output.
    vtkRectilinearGrid *rgds=dynamic_cast<vtkRectilinearGrid*>(output);

    vtkFloatArray *fa;
    float *pFa;
    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=md->GetCoordinate(0)->GetPointer()[ilo];
    pFa[1]=md->GetCoordinate(0)->GetPointer()[ihi];
    rgds->SetXCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=((float)subsetBounds[2]);
    pFa[1]=((float)subsetBounds[3]);
    rgds->SetYCoordinates(fa);
    fa->Delete();

    fa=vtkFloatArray::New();
    fa->SetNumberOfTuples(2);
    pFa=fa->GetPointer(0);
    pFa[0]=((float)subsetBounds[4]);
    pFa[1]=((float)subsetBounds[5]);
    rgds->SetZCoordinates(fa);
    fa->Delete();

    rgds->SetExtent(decomp.GetData());

    // Setup the user defined domain decomposition over the subset. This
    // decomposition is used to fine tune the I/O performance of out-of-core
    // filters.
    RectilinearDecomp *rddecomp=RectilinearDecomp::New();
    rddecomp->SetFileExtent(fileExt);
    rddecomp->SetExtent(subset);
    rddecomp->SetDecompDims(this->DecompDims);
    rddecomp->SetPeriodicBC(this->PeriodicBC);
    rddecomp->SetNumberOfGhostCells(this->NGhosts);
    rddecomp->SetCoordinate(0,md->GetCoordinate(0));
    rddecomp->SetCoordinate(1,md->GetCoordinate(1));
    rddecomp->SetCoordinate(2,md->GetCoordinate(2));
    int ok=rddecomp->DecomposeDomain();
    if (!ok)
      {
      vtkErrorMacro("Failed to decompose domain.");
      output->Initialize();
      return 1;
      }
    ddecomp=rddecomp;
    }
  else
  if (this->Reader->DataSetTypeIsStructured())
    {
    /// Structured data
    vtkErrorMacro("vtkStructuredData is not implemented yet.");
    return 1;
    }
  else
    {
    /// unrecognized dataset type
    vtkErrorMacro(
      << "Error: invalid dataset type \""
      << md->GetDataSetType() << "\".");
    }

  // Pseduo read puts place holders for the selected arrays into
  // the output so they can be selected in downstream filters' GUIs.
  int ok;
  ok=this->Reader->ReadMetaTimeStep(stepId,output,this);
  if (!ok)
    {
    vtkErrorMacro("Read failed." << endl << *md);
    output->Initialize();
    return 1;
    }

  // Put a reader into the pipeline, downstream filters can
  // the read on demand.
  vtkSQOOCBOVReader *OOCReader=vtkSQOOCBOVReader::New();
  OOCReader->SetReader(this->Reader);
  OOCReader->SetTimeIndex(stepId);
  OOCReader->SetDomainDecomp(ddecomp);
  OOCReader->SetBlockCacheSize(this->BlockCacheSize);
  OOCReader->SetCloseClearsCachedBlocks(this->ClearCachedBlocks);
  OOCReader->InitializeBlockCache();
  OOCReader->SetLogLevel(this->LogLevel);
  info->Set(vtkSQOOCReader::READER(),OOCReader);
  OOCReader->Delete();
  if(ddecomp) ddecomp->Delete();
  req->Append(vtkExecutive::KEYS_TO_COPY(),vtkSQOOCReader::READER());

  // Give implementation classes a chance to store specialized keys
  // into the pipeline.
  md->PushPipelineInformation(req, info);

  #if defined SQTK_DEBUG
  this->Reader->PrintSelf(oss);
  output->Print(oss);
  pCerr() << oss << std::endl;
  #endif

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQBOVMetaReader::RequestData");
    }

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBOVMetaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  os << "vtkSQBOVMetaReader" << std::endl;
  // TODO
  this->Superclass::PrintSelf(os,indent);
  os << std::endl;
}
