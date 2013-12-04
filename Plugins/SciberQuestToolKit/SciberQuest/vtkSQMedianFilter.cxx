/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQMedianFilter.h"

// #define SQTK_DEBUG
#include "SQPOSIXOnWindowsWarningSupression.h"
#include "SQVTKTemplateMacroWarningSupression.h"
#include "SQPosixOnWindows.h"
#include "Numerics.hxx"
#include "MemOrder.hxx"
#include "Tuple.hxx"
#include "CartesianExtent.h"
#include "CartesianExtent.h"
#include "CPUConvolutionDriver.h"
#include "CUDAConvolutionDriver.h"
#include "vtkSQLog.h"
#include "XMLUtils.h"
#include "postream.h"
#include "SQMacros.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkImageData.h"
#include "vtkRectilinearGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkFloatArray.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"

#include <cstdlib>
#include <string>
#include <map>
#include <vector>
#include <utility>
#include <algorithm>

vtkStandardNewMacro(vtkSQMedianFilter);

//-----------------------------------------------------------------------------
vtkSQMedianFilter::vtkSQMedianFilter()
    :
  //WorldSize(1),
  //WorldRank(0),
  //HostSize(1),
  //HostRank(0),
  KernelWidth(3),
  KernelType(KERNEL_TYPE_MEDIAN),
  //Kernel(0),
  KernelModified(1),
  Mode(CartesianExtent::DIM_MODE_3D),
  //NumberOfCUDADevices(0),
  //NumberOfActiveCUDADevices(0),
  //CUDADeviceId(-1),
  //NumberOfMPIRanksToUseCUDA(0),
  //EnableCUDA(0),
  LogLevel(0)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::vtkSQMedianFilter" << std::endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  /*
  // may be a parallel run, we need to determine how
  // many of the ranks are running on each host.
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    const int managementRank=0;

    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);

    const int hostNameLen=512;
    char hostName[hostNameLen]={'\0'};
    gethostname(hostName,hostNameLen);

    char *hostNames=0;
    if (this->WorldRank==managementRank)
      {
      hostNames=(char*)malloc(this->WorldSize*hostNameLen);
      }
    MPI_Gather(
        hostName,
        hostNameLen,
        MPI_CHAR,
        hostNames,
        hostNameLen,
        MPI_CHAR,
        managementRank,
        MPI_COMM_WORLD);
    int *hostSizes=0;
    int *hostRanks=0;
    if (this->WorldRank==managementRank)
      {
      hostRanks=(int*)malloc(this->WorldSize*sizeof(int));

      std::vector<std::string> keys(this->WorldSize);
      typedef std::map<std::string,int> CountMapType;
      typedef CountMapType::iterator CountMapItType;
      typedef std::pair<CountMapItType,bool> CountMapInsType;
      typedef std::pair<const std::string,int> CountMapValType;
      CountMapType counts;

      std::pair<const std::string,int> val;
      for (int i=0; i<this->WorldSize; ++i)
        {
        keys[i]=hostNames[i*hostNameLen];

        CountMapInsType ret;
        CountMapValType val(keys[i],0);
        ret=counts.insert(val);
        if (ret.second)
          {
          ret.first->second=1;
          }
        else
          {
          ret.first->second+=1;
          }
        hostRanks[i]=ret.first->second-1;
        }
      hostSizes=(int*)malloc(this->WorldSize*sizeof(int));
      for (int i=0; i<this->WorldSize; ++i)
        {
        hostSizes[i]=counts[keys[i]];
        }
      }
    MPI_Scatter(
          hostSizes,
          1,
          MPI_INT,
          &this->HostSize,
          1,
          MPI_INT,
          managementRank,
          MPI_COMM_WORLD);
    MPI_Scatter(
          hostRanks,
          1,
          MPI_INT,
          &this->HostRank,
          1,
          MPI_INT,
          managementRank,
          MPI_COMM_WORLD);
    if (this->WorldRank==managementRank)
      {
      free(hostNames);
      free(hostSizes);
      free(hostRanks);
      }
    }

  // inti cpu driver
  this->CPUDriver=new CPUConvolutionDriver;

  // inti cuda driver
  this->CUDADeviceRange[0]=0;
  this->CUDADeviceRange[1]=0;

  this->CUDADriver=new CUDAConvolutionDriver;
  this->CUDADriver->SetNumberOfWarpsPerBlock(1);
  this->NumberOfCUDADevices=this->CUDADriver->GetNumberOfDevices();
  if (this->NumberOfCUDADevices)
    {
    int ierr=this->SetCUDADeviceId(0);
    if (ierr)
      {
      sqErrorMacro(pCerr(),"Failed to select CUDA device 0.");
      return;
      }
    this->CUDADeviceRange[1]=this->NumberOfCUDADevices-1;
    }
  this->SetNumberOfActiveCUDADevices(this->NumberOfCUDADevices);

  #ifdef SQTK_DEBUG
  pCerr() << "HostSize=" << this->HostSize << std::endl;
  pCerr() << "HostRank=" << this->HostRank << std::endl;
  #endif
  */
}

//-----------------------------------------------------------------------------
vtkSQMedianFilter::~vtkSQMedianFilter()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::~vtkSQMedianFilter" << std::endl;
  #endif

  /*
  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
    }

  delete this->CPUDriver;
  delete this->CUDADriver;
  */
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::Initialize(vtkPVXMLElement *root)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQMedianFilter");
  if (elem==0)
    {
    //sqErrorMacro(pCerr(),"Element for vtkSQMedianFilter is not present.");
    return -1;
    }

  int stencilWidth=0;
  GetOptionalAttribute<int,1>(elem,"stencilWidth",&stencilWidth);
  if (stencilWidth>2)
    {
    this->SetKernelWidth(stencilWidth);
    }

  int kernelType=-1;
  GetOptionalAttribute<int,1>(elem,"kernelType",&kernelType);
  if (kernelType>=0)
    {
    this->SetKernelType(kernelType);
    }

  /*
  int CPUDriverOptimization=-1;
  GetOptionalAttribute<int,1>(elem,"CPUDriverOptimization",&CPUDriverOptimization);
  if (CPUDriverOptimization>=0)
    {
    this->SetCPUDriverOptimization(CPUDriverOptimization);
    }

  int numberOfMPIRanksToUseCUDA=0;
  GetOptionalAttribute<int,1>(elem,"numberOfMPIRanksToUseCUDA",&numberOfMPIRanksToUseCUDA);
  */

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    vtkSQLog *log=vtkSQLog::GetGlobalInstance();
    log->GetHeader()
      << "# ::vtkSQMedianFilter" << "\n"
      << "#   stencilWidth=" << stencilWidth << "\n"
      << "#   kernelType=" << kernelType << "\n";
      //<< "#   CPUDriverOptimization=" << CPUDriverOptimization << "\n"
      //<< "#   numberOfMPIRanksToUseCUDA=" << numberOfMPIRanksToUseCUDA << "\n";
    }

  /*
  if (numberOfMPIRanksToUseCUDA)
    {
    this->SetNumberOfMPIRanksToUseCUDA(numberOfMPIRanksToUseCUDA);

    int numberOfActiveCUDADevices=1;
    GetOptionalAttribute<int,1>(elem,"numberOfActiveCUDADevices",&numberOfActiveCUDADevices);
    this->SetNumberOfActiveCUDADevices(numberOfActiveCUDADevices);

    int numberOfWarpsPerCUDABlock=0;
    GetOptionalAttribute<int,1>(elem,"numberOfWarpsPerCUDABlock",&numberOfWarpsPerCUDABlock);
    if (numberOfWarpsPerCUDABlock)
      {
      this->SetNumberOfWarpsPerCUDABlock(numberOfWarpsPerCUDABlock);
      }

    int kernelCUDAMemType=-1;
    GetOptionalAttribute<int,1>(elem,"kernelCUDAMemoryType",&kernelCUDAMemType);
    if (kernelCUDAMemType>=0)
      {
      this->SetKernelCUDAMemoryType(kernelCUDAMemType);
      }

    int inputCUDAMemType=-1;
    GetOptionalAttribute<int,1>(elem,"inputCUDAMemoryType",&inputCUDAMemType);
    if (inputCUDAMemType>=0)
      {
      this->SetInputCUDAMemoryType(inputCUDAMemType);
      }

    #if defined vtkSQMedianFilterTIME
    vtkSQLog *log=vtkSQLog::GetGlobalInstance();
    *log
      << "#   numberOfActiveCUDADevices=" << numberOfActiveCUDADevices << "\n"
      << "#   numberOfWarpsPerCUDABlock=" << numberOfWarpsPerCUDABlock << "\n"
      << "#   kernelCUDAMemType=" << kernelCUDAMemType << "\n"
      << "#   inputCUDAMemType=" << inputCUDAMemType << "\n"
      << "\n";
    #endif
    }
  */

  return 0;
}

/*
//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetCPUDriverOptimization(int opt)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetCPUDriverOptimization"
    << " " << opt << std::endl;
  #endif
  this->CPUDriver->SetOptimization(opt);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::GetCPUDriverOptimization()
{
  return this->CPUDriver->GetOptimization();
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetAllMPIRanksToUseCUDA(int allUse)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetAllMPIRanksToUseCUDA"
    << " " << allUse
    << std::endl;
  #endif

  if (allUse && this->NumberOfActiveCUDADevices)
    {
    this->EnableCUDA=1;
    }
  else
    {
    this->EnableCUDA=0;
    }

  this->Modified();

  #ifdef SQTK_DEBUG
  pCerr() << "EnableCUDA=" << this->EnableCUDA << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetNumberOfMPIRanksToUseCUDA(int nRanks)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetNumberOfMPIRanksToUseCUDA"
    << " " << nRanks
    << std::endl;
  #endif
  if (nRanks==this->NumberOfMPIRanksToUseCUDA)
    {
    return;
    }
  //nRanks=max(0,nRanks);
  //nRanks=min(nRanks,this->HostSize);
  this->NumberOfMPIRanksToUseCUDA=nRanks;

  if (nRanks==-1)
    {
    this->SetAllMPIRanksToUseCUDA(1);
    return;
    }

  if ( this->NumberOfActiveCUDADevices
    && (this->HostRank<this->NumberOfMPIRanksToUseCUDA))
    {
    // run on GPU
    this->EnableCUDA=1;
    }
  else
    {
    // run on CPU
    this->EnableCUDA=0;
    }

  this->Modified();

  #ifdef SQTK_DEBUG
  pCerr() << "EnableCUDA=" << this->EnableCUDA << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetNumberOfActiveCUDADevices(int nActive)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetNumberOfActiveCUDADevices"
    << " " << nActive
    << std::endl;
  #endif

  //nActive=max(0,nActive);
  nActive=std::min(nActive,this->NumberOfCUDADevices);
  if (nActive==this->NumberOfActiveCUDADevices)
    {
    return;
    }

  // interpret -1 to mean use all available
  if (nActive==-1)
    {
    this->NumberOfActiveCUDADevices=this->NumberOfCUDADevices;
    }
  else
    {
    this->NumberOfActiveCUDADevices=nActive;
    }

  // determine which device this rank will run on.
  if (this->NumberOfActiveCUDADevices)
    {
    int deviceId=this->HostRank%this->NumberOfActiveCUDADevices;
    this->SetCUDADeviceId(deviceId);
    #ifdef SQTK_DEBUG
    pCerr() << "assigned to cuda device " << deviceId << std::endl;
    #endif
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::SetCUDADeviceId(int deviceId)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetCUDADeviceId"
    << " " << deviceId
    << std::endl;
  #endif
  if (this->CUDADeviceId==deviceId)
    {
    return 0;
    }

  this->Modified();
  this->CUDADeviceId=deviceId;

  return this->CUDADriver->SetDeviceId(deviceId);
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetKernelCUDAMemoryType(int memType)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetKernelCUDAMemoryType"
    << " " << memType << std::endl;
  #endif
  this->CUDADriver->SetKernelMemoryType(memType);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::GetKernelCUDAMemoryType()
{
  return this->CUDADriver->GetKernelMemoryType();
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetInputCUDAMemoryType(int memType)
{
  #ifdef SQTK_DEBUG
  pCerr()
    << "=====vtkSQMedianFilter::SetInputCUDAMemoryType"
    << " " << memType << std::endl;
  #endif
  this->CUDADriver->SetInputMemoryType(memType);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::GetInputCUDAMemoryType()
{
  return this->CUDADriver->GetInputMemoryType();
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetNumberOfWarpsPerCUDABlock(int nWarpsPer)
{
  this->CUDADriver->SetNumberOfWarpsPerBlock(nWarpsPer);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::GetNumberOfWarpsPerCUDABlock()
{
  return this->CUDADriver->GetNumberOfWarpsPerBlock();
}
*/

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetMode(int mode)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::SetMode" << std::endl;
  #endif

  if (mode==this->Mode)
    {
    return;
    }

  this->Mode=mode;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetKernelType(int type)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::SetKernelType" << std::endl;
  #endif

  if (type==this->KernelType)
    {
    return;
    }

  this->KernelType=type;
  this->Modified();
  //this->KernelModified=1;
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::SetKernelWidth(int width)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::SetKernelWidth" << std::endl;
  #endif

  if (width==this->KernelWidth)
    {
    return;
    }

  if ((this->KernelWidth%2)==0)
    {
    vtkErrorMacro("KernelWidth must be odd.");
    return;
    }

  this->KernelWidth=width;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::UpdateKernel()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::UpdateKernel" << std::endl;
  #endif

  if (!this->KernelModified)
    {
    return 0;
    }

  int nk2 = this->KernelWidth/2;
  CartesianExtent ext(-nk2, nk2, -nk2, nk2, -nk2, nk2);
  switch(this->Mode)
    {
    case CartesianExtent::DIM_MODE_2D_XY:
      ext[4]=0;
      ext[5]=0;
      break;
    case CartesianExtent::DIM_MODE_2D_XZ:
      ext[2]=0;
      ext[3]=0;
      break;
    case CartesianExtent::DIM_MODE_2D_YZ:
      ext[0]=0;
      ext[1]=0;
      break;
    case CartesianExtent::DIM_MODE_3D:
      break;
    }
  this->KernelExt=ext;

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::RequestDataObject" << std::endl;
  #endif


  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());
  const char *inputType=inData->GetClassName();

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  if ( !outData || !outData->IsA(inputType))
    {
    outData=inData->NewInstance();
    outInfo->Set(vtkDataObject::DATA_TYPE_NAME(),inputType);
    outInfo->Set(vtkDataObject::DATA_OBJECT(),outData);
    outInfo->Set(vtkDataObject::DATA_EXTENT_TYPE(), inData->GetExtentType());
    outData->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::RequestInformation" << std::endl;
  #endif
  //this->Superclass::RequestInformation(req,inInfos,outInfos);

  // We will work in a restricted problem domain so that we have
  // always a single layer of ghost cells available. To make it so
  // we'll take the upstream's domain and shrink it by half the
  // kernel width.
  int nGhosts = this->KernelWidth/2;

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);
  CartesianExtent inputDomain;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDomain.GetData());

  // determine the dimensionality of the input.
  this->Mode
    = CartesianExtent::GetDimensionMode(
          inputDomain,
          nGhosts);
  if (this->Mode==CartesianExtent::DIM_MODE_INVALID)
    {
    vtkErrorMacro("Invalid problem domain.");
    }

  // shrink the output problem domain by the requisite number of
  // ghost cells.
  CartesianExtent outputDomain
    = CartesianExtent::Grow(
          inputDomain,
          -nGhosts,
          this->Mode);

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  outInfo->Set(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        outputDomain.GetData(),
        6);

  // other keys that need to be coppied
  double dX[3];
  inInfo->Get(vtkDataObject::SPACING(),dX);
  outInfo->Set(vtkDataObject::SPACING(),dX,3);

  double X0[3];
  inInfo->Get(vtkDataObject::ORIGIN(),X0);
  outInfo->Set(vtkDataObject::ORIGIN(),X0,3);

  #ifdef SQTK_DEBUG
  pCerr()
    << "WHOLE_EXTENT(input)=" << inputDomain << std::endl
    << "WHOLE_EXTENT(output)=" << outputDomain << std::endl
    << "ORIGIN=" << Tuple<double>(X0,3) << std::endl
    << "SPACING=" << Tuple<double>(dX,3) << std::endl
    << "nGhost=" << nGhosts << std::endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::RequestUpdateExtent" << std::endl;
  #endif

  (void)req;

  typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;

  vtkInformation* outInfo=outInfos->GetInformationObject(0);
  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  // We will modify the extents we request from our input so
  // that we will have a layers of ghost cells. We also pass
  // the number of ghosts through the piece based key.
  int nGhosts = this->KernelWidth/2;

  inInfo->Set(
        vtkSDDPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
        nGhosts);

  CartesianExtent outputExt;
  outInfo->Get(
        vtkSDDPipeline::UPDATE_EXTENT(),
        outputExt.GetData());

  CartesianExtent wholeExt;
  inInfo->Get(
        vtkSDDPipeline::WHOLE_EXTENT(),
        wholeExt.GetData());

  outputExt = CartesianExtent::Grow(
        outputExt,
        wholeExt,
        nGhosts,
        this->Mode);

  inInfo->Set(
        vtkSDDPipeline::UPDATE_EXTENT(),
        outputExt.GetData(),
        6);

  int piece
    = outInfo->Get(vtkSDDPipeline::UPDATE_PIECE_NUMBER());

  int numPieces
    = outInfo->Get(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES());

  inInfo->Set(vtkSDDPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(vtkSDDPipeline::EXACT_EXTENT(), 1);

  #ifdef SQTK_DEBUG
  pCerr()
    << "WHOLE_EXTENT=" << wholeExt << std::endl
    << "UPDATE_EXTENT=" << outputExt << std::endl
    << "nGhosts=" << nGhosts << std::endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQMedianFilter::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::RequestData" << std::endl;
  #endif

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->StartEvent("vtkSQMedianFilter::RequestData");
    }

  vtkInformation *inInfo=inInfoVec[0]->GetInformationObject(0);
  vtkDataObject *inData=inInfo->Get(vtkDataObject::DATA_OBJECT());

  vtkInformation *outInfo=outInfoVec->GetInformationObject(0);
  vtkDataObject *outData=outInfo->Get(vtkDataObject::DATA_OBJECT());

  // Guard against empty input.
  if (!inData || !outData)
    {
    vtkErrorMacro(
      << "Empty input(" << inData << ") or output(" << outData << ") detected.");
    return 1;
    }
  // We need extent based data here.
  int isImage=inData->IsA("vtkImageData");
  int isRecti=inData->IsA("vtkrectilinearGrid");
  if (!isImage && !isRecti)
    {
    vtkErrorMacro(
      << "This filter is designed for vtkStructuredData and subclasses."
      << "You are trying to use it with " << inData->GetClassName() << ".");
    return 1;
    }

  // Get the input and output extents.
  CartesianExtent extV;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        extV.GetData());

  CartesianExtent inputDom;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDom.GetData());

  CartesianExtent extW;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        extW.GetData());

  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  // Check that we have the ghost cells that we need (more is OK).
  int nGhost = this->KernelWidth/2;

  CartesianExtent inputBox(extV);
  CartesianExtent outputBox
    = CartesianExtent::Grow(extW, nGhost, this->Mode);

  if (!inputBox.Contains(outputBox))
    {
    vtkErrorMacro(
      << "This filter requires ghost cells to function correctly. "
      << "The input must conatin the output plus " << nGhost
      << " layers of ghosts. The input is " << inputBox
      << ", but it must be at least "
      << outputBox << ".");
    return 1;
    }

  // generate the requested kernel, if needed.
  if (this->UpdateKernel())
    {
    vtkErrorMacro("Failed to create the requested kernel.");
    return 1;
    }

  // NOTE You can't do a shallow copy because the array dimensions are
  // different on output and input because of the ghost layers.

  if (isImage)
    {
    vtkImageData *inImData=dynamic_cast<vtkImageData *>(inData);
    vtkImageData *outImData=dynamic_cast<vtkImageData *>(outData);

    // set up the output.
    double X0[3];
    outInfo->Get(vtkDataObject::ORIGIN(),X0);
    outImData->SetOrigin(X0);

    double dX[3];
    outInfo->Get(vtkDataObject::SPACING(),dX);
    outImData->SetSpacing(dX);

    outImData->SetExtent(extW.GetData());

    int outputDims[3];
    outImData->GetDimensions(outputDims);
    vtkIdType outputTups=outputDims[0]*outputDims[1]*outputDims[2];

    #ifdef SQTK_DEBUG
    pCerr()
      << "WHOLE_EXTENT=" << domainExt << std::endl
      << "UPDATE_EXTENT(input)=" << extV << std::endl
      << "UPDATE_EXTENT(output)=" << extW << std::endl
      << "ORIGIN" << Tuple<double>(X0,3) << std::endl
      << "SPACING" << Tuple<double>(dX,3) << std::endl
      << std::endl;
    #endif

    vtkDataArray *V=this->GetInputArrayToProcess(0,inImData);

    if (!V->IsA("vtkFloatArray") && !V->IsA("vtkDoubleArray"))
      {
      vtkErrorMacro(
        << "This filter operates on vector floating point arrays."
        << "You provided " << V->GetClassName() << ".");
      return 1;
      }

    int nComps = V->GetNumberOfComponents();

    vtkDataArray *W=V->NewInstance();
    W->SetNumberOfComponents(nComps);
    W->SetNumberOfTuples(outputTups);
    W->SetName(V->GetName());

    int nV[3];
    extV.Size(nV);
    size_t vnijk=extV.Size();

    int nW[3];
    extW.Size(nW);
    size_t wnijk=extW.Size();
    size_t nComp=W->GetNumberOfComponents();

    int nK[3];
    this->KernelExt.Size(nK);
    size_t knijk=this->KernelExt.Size();

    int fastDim=0;
    int slowDim=1;
    switch (this->Mode)
      {
      case CartesianExtent::DIM_MODE_2D_XY:
        fastDim=0;
        slowDim=1;
        break;
      case CartesianExtent::DIM_MODE_2D_XZ:
        fastDim=0;
        slowDim=2;
        break;
      case CartesianExtent::DIM_MODE_2D_YZ:
        fastDim=1;
        slowDim=2;
        break;
      case CartesianExtent::DIM_MODE_3D:
        fastDim=0;
        slowDim=1;
        break;
      default:
        sqErrorMacro(std::cerr,"Bad dim mode.");
        return -1;
      }

    #ifdef SQTK_DEBUG
    pCerr() << "wnijk=" << wnijk << std::endl;
    pCerr() << "fastDim=" << fastDim << std::endl;
    pCerr() << "slowDim=" << slowDim << std::endl;
    pCerr() << "extV=" << extV << std::endl;
    pCerr() << "nV=(" << nV[fastDim] <<  ", " << nV[slowDim] << ")" << std::endl;
    pCerr() << "extW=" << extW << std::endl;
    pCerr() << "nW=(" << nW[fastDim] <<  ", " << nW[slowDim] << ")" << std::endl;
    #endif

    switch (V->GetDataType())
      {
      // TODO -- replace with vtkTemplateMacro
      case VTK_FLOAT:
        {
        std::vector<float*> sV(nComp,0);
        std::vector<float*> sW(nComp,0);
        for (unsigned long q=0; q<nComp; ++q)
          {
          posix_memalign((void**)&sV[q],16,vnijk*sizeof(float));
          posix_memalign((void**)&sW[q],16,wnijk*sizeof(float));
          }

        // convert vtk vectors/tensors into scalar component arrays
        float *hV=(float*)V->GetVoidPointer(0);
        Split<float>(vnijk,hV,sV);

        // apply convolution
        for (unsigned long q=0; q<nComp; ++q)
          {
          if ((this->Mode==CartesianExtent::DIM_MODE_2D_XY)
            ||(this->Mode==CartesianExtent::DIM_MODE_2D_XZ)
            ||(this->Mode==CartesianExtent::DIM_MODE_2D_YZ))
            {
            ::ScalarMedianFilter2D(
                  nV[fastDim],
                  nW[fastDim],
                  wnijk,
                  nK[fastDim],
                  knijk,
                  nGhost,
                  sV[q],
                  sW[q]);
            }
          else
            {
            ::ScalarMedianFilter3D(
                  nV[fastDim],
                  nV[fastDim]*nV[slowDim],
                  nW[fastDim],
                  nW[fastDim]*nW[slowDim],
                  wnijk,
                  nK[fastDim],
                  nK[fastDim]*nK[slowDim],
                  knijk,
                  nGhost,
                  sV[q],
                  sW[q]);
            }
          }

        // put results in vtk order
        float *hW=(float*)W->GetVoidPointer(0);
        Interleave(wnijk,sW,hW);

        // clean up
        for (unsigned long q=0; q<nComp; ++q)
          {
          free(sW[q]);
          free(sV[q]);
          }
        }
        break;
      default:
        // TODO
        sqErrorMacro(std::cerr,"Not currently using vtkTemplateMacro");
        return -1;
      }

    outImData->GetPointData()->AddArray(W);
    W->Delete();

    // outImData->Print(std::cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

  if (this->LogLevel || globalLogLevel)
    {
    log->EndEvent("vtkSQMedianFilter::RequestData");
    }

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQMedianFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQMedianFilter::PrintSelf" << std::endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}
