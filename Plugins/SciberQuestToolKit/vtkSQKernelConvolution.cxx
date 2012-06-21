/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2012 SciberQuest Inc.

*/
#include "vtkSQKernelConvolution.h"

// #define vtkSQKernelConvolutionDEBUG
// #define vtkSQKernelConvolutionTIME

#if defined vtkSQKernelConvolutionTIME
  #include "vtkSQLog.h"
#endif

#include "Numerics.hxx"
#include "Tuple.hxx"
#include "CartesianExtent.h"
#include "CartesianExtent.h"
#include "CPUConvolutionDriver.h"
#include "CUDAConvolutionDriver.h"
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

#ifdef WIN32
  #include <Winsock2.h>
#else
  #include <unistd.h>
#endif

#include <string>
using std::string;
#include <map>
using std::map;
#include <vector>
using std::vector;
#include <utility>
using std::pair;
#include <algorithm>
using std::min;
using std::max;


#ifndef SQTK_WITHOUT_MPI
#include <mpi.h>
#endif

vtkStandardNewMacro(vtkSQKernelConvolution);

//-----------------------------------------------------------------------------
vtkSQKernelConvolution::vtkSQKernelConvolution()
    :
  WorldSize(1),
  WorldRank(0),
  HostSize(1),
  HostRank(0),
  KernelWidth(3),
  KernelType(KERNEL_TYPE_GAUSIAN),
  Kernel(0),
  KernelModified(1),
  Mode(CartesianExtent::DIM_MODE_3D),
  NumberOfCUDADevices(0),
  NumberOfActiveCUDADevices(0),
  CUDADeviceId(-1),
  NumberOfMPIRanksToUseCUDA(0),
  EnableCUDA(0)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::vtkSQKernelConvolution" << endl;
  #endif

  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  #ifndef SQTK_WITHOUT_MPI
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

      vector<string> keys(this->WorldSize);
      typedef map<string,int> CountMapType;
      typedef CountMapType::iterator CountMapItType;
      typedef pair<CountMapItType,bool> CountMapInsType;
      typedef pair<const string,int> CountMapValType;
      CountMapType counts;

      pair<const string,int> val;
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
  #endif

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

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "HostSize=" << this->HostSize << endl;
  pCerr() << "HostRank=" << this->HostRank << endl;
  #endif
}

//-----------------------------------------------------------------------------
vtkSQKernelConvolution::~vtkSQKernelConvolution()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::~vtkSQKernelConvolution" << endl;
  #endif

  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
    }

  delete this->CPUDriver;
  delete this->CUDADriver;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::Initialize(vtkPVXMLElement *root)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::Initialize" << endl;
  #endif

  vtkPVXMLElement *elem=0;
  elem=GetOptionalElement(root,"vtkSQKernelConvolution");
  if (elem==0)
    {
    //sqErrorMacro(pCerr(),"Element for vtkSQKernelConvolution is not present.");
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

  int CPUDriverOptimization=-1;
  GetOptionalAttribute<int,1>(elem,"CPUDriverOptimization",&CPUDriverOptimization);
  if (CPUDriverOptimization>=0)
    {
    this->SetCPUDriverOptimization(CPUDriverOptimization);
    }

  int numberOfMPIRanksToUseCUDA=0;
  GetOptionalAttribute<int,1>(elem,"numberOfMPIRanksToUseCUDA",&numberOfMPIRanksToUseCUDA);

  #if defined vtkSQKernelConvolutionTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQKernelConvolution" << "\n"
    << "#   stencilWidth=" << stencilWidth << "\n"
    << "#   kernelType=" << kernelType << "\n"
    << "#   CPUDriverOptimization=" << CPUDriverOptimization << "\n"
    << "#   numberOfMPIRanksToUseCUDA=" << numberOfMPIRanksToUseCUDA << "\n";
  #endif

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

    #if defined vtkSQKernelConvolutionTIME
    vtkSQLog *log=vtkSQLog::GetGlobalInstance();
    *log
      << "#   numberOfActiveCUDADevices=" << numberOfActiveCUDADevices << "\n"
      << "#   numberOfWarpsPerCUDABlock=" << numberOfWarpsPerCUDABlock << "\n"
      << "#   kernelCUDAMemType=" << kernelCUDAMemType << "\n"
      << "#   inputCUDAMemType=" << inputCUDAMemType << "\n"
      << "\n";
    #endif
    }

  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::AddInputArray(const char *name)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::AddInputArray"
    << "name=" << name << endl;
  #endif

  if (this->InputArrays.insert(name).second)
    {
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::ClearInputArrays()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::ClearInputArrays" << endl;
  #endif

  if (this->InputArrays.size())
    {
    this->InputArrays.clear();
    this->Modified();
    }
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetCPUDriverOptimization(int opt)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetCPUDriverOptimization"
    << " " << opt << endl;
  #endif
  this->CPUDriver->SetOptimization(opt);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::GetCPUDriverOptimization()
{
  return this->CPUDriver->GetOptimization();
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetAllMPIRanksToUseCUDA(int allUse)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetAllMPIRanksToUseCUDA"
    << " " << allUse
    << endl;
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

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "EnableCUDA=" << this->EnableCUDA << endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetNumberOfMPIRanksToUseCUDA(int nRanks)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetNumberOfMPIRanksToUseCUDA"
    << " " << nRanks
    << endl;
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

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "EnableCUDA=" << this->EnableCUDA << endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetNumberOfActiveCUDADevices(int nActive)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetNumberOfActiveCUDADevices"
    << " " << nActive
    << endl;
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
    #ifdef vtkSQKernelConvolutionDEBUG
    pCerr() << "assigned to cuda device " << deviceId << endl;
    #endif
    }

  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::SetCUDADeviceId(int deviceId)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetCUDADeviceId"
    << " " << deviceId
    << endl;
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
void vtkSQKernelConvolution::SetKernelCUDAMemoryType(int memType)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetKernelCUDAMemoryType"
    << " " << memType << endl;
  #endif
  this->CUDADriver->SetKernelMemoryType(memType);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::GetKernelCUDAMemoryType()
{
  return this->CUDADriver->GetKernelMemoryType();
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetInputCUDAMemoryType(int memType)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "=====vtkSQKernelConvolution::SetInputCUDAMemoryType"
    << " " << memType << endl;
  #endif
  this->CUDADriver->SetInputMemoryType(memType);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::GetInputCUDAMemoryType()
{
  return this->CUDADriver->GetInputMemoryType();
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetNumberOfWarpsPerCUDABlock(int nWarpsPer)
{
  this->CUDADriver->SetNumberOfWarpsPerBlock(nWarpsPer);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::GetNumberOfWarpsPerCUDABlock()
{
  return this->CUDADriver->GetNumberOfWarpsPerBlock();
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetMode(int mode)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::SetMode" << endl;
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
void vtkSQKernelConvolution::SetKernelType(int type)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::SetKernelType" << endl;
  #endif

  if (type==this->KernelType)
    {
    return;
    }

  this->KernelType=type;
  this->Modified();
  this->KernelModified=1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::SetKernelWidth(int width)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::SetKernelWidth" << endl;
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
int vtkSQKernelConvolution::UpdateKernel()
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::UpdateKernel" << endl;
  #endif

  if (!this->KernelModified)
    {
    return 0;
    }

  if (this->Kernel)
    {
    delete [] this->Kernel;
    this->Kernel=0;
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

  int size = ext.Size();

  this->Kernel=new float [size];
  float kernelNorm=0.0;

  if (this->KernelType==KERNEL_TYPE_GAUSIAN)
    {
    float *X=new float[this->KernelWidth];
    linspace<float>(-1.0,1.0, this->KernelWidth, X);

    float B[3]={0.0,0.0,0.0};
    float a=1.0;
    float c=0.55;

    int H=(this->Mode==CartesianExtent::DIM_MODE_3D?this->KernelWidth:1);

    for (int k=0; k<H; ++k)
      {
      for (int j=0; j<this->KernelWidth; ++j)
        {
        for (int i=0; i<this->KernelWidth; ++i)
          {
          float x[3]={X[i],X[j],this->Mode==CartesianExtent::DIM_MODE_3D?X[k]:0.0};

          int q = this->KernelWidth*this->KernelWidth*k+this->KernelWidth*j+i;

          this->Kernel[q]=Gaussian(x,a,B,c);
          kernelNorm+=this->Kernel[q];
          }
        }
      }
    delete [] X;
    }
  else
  if (this->KernelType==KERNEL_TYPE_CONSTANT)
    {
    kernelNorm=size;
    for (int i=0; i<size; ++i)
      {
      this->Kernel[i]=1.0;
      }
    }
  else
    {
    vtkErrorMacro("Unsupported KernelType " << this->KernelType << ".");
    delete [] this->Kernel;
    this->Kernel=0;
    return -1;
    }

  // normalize
  for (int i=0; i<size; ++i)
    {
    this->Kernel[i]/=kernelNorm;
    }

  this->KernelModified = 0;

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "Kernel=[";
  for (int i=0; i<size; ++i)
    {
    cerr << this->Kernel[i] << " ";
    }
  cerr << "]" << endl;
  #endif

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestDataObject(
    vtkInformation* /* request */,
    vtkInformationVector** inInfoVec,
    vtkInformationVector* outInfoVec)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::RequestDataObject" << endl;
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
int vtkSQKernelConvolution::RequestInformation(
      vtkInformation * /*req*/,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::RequestInformation" << endl;
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

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "WHOLE_EXTENT(input)=" << inputDomain << endl
    << "WHOLE_EXTENT(output)=" << outputDomain << endl
    << "ORIGIN=" << Tuple<double>(X0,3) << endl
    << "SPACING=" << Tuple<double>(dX,3) << endl
    << "nGhost=" << nGhosts << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::RequestUpdateExtent" << endl;
  #endif

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

  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << wholeExt << endl
    << "UPDATE_EXTENT=" << outputExt << endl
    << "nGhosts=" << nGhosts << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQKernelConvolution::RequestData(
    vtkInformation * /*req*/,
    vtkInformationVector **inInfoVec,
    vtkInformationVector *outInfoVec)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::RequestData" << endl;
  #endif

  #if defined vtkSQKernelConvolutionTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQKernelConvolution::RequestData");
  #endif

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
  CartesianExtent inputExt;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        inputExt.GetData());

  CartesianExtent inputDom;
  inInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        inputDom.GetData());

  CartesianExtent outputExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        outputExt.GetData());

  CartesianExtent domainExt;
  outInfo->Get(
        vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
        domainExt.GetData());

  // Check that we have the ghost cells that we need (more is OK).
  int nGhost = this->KernelWidth/2;

  CartesianExtent inputBox(inputExt);
  CartesianExtent outputBox
    = CartesianExtent::Grow(outputExt, nGhost, this->Mode);

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

    outImData->SetExtent(outputExt.GetData());

    int outputDims[3];
    outImData->GetDimensions(outputDims);
    int outputTups=outputDims[0]*outputDims[1]*outputDims[2];

    #ifdef vtkSQKernelConvolutionDEBUG
    pCerr()
      << "WHOLE_EXTENT=" << domainExt << endl
      << "UPDATE_EXTENT(input)=" << inputExt << endl
      << "UPDATE_EXTENT(output)=" << outputExt << endl
      << "ORIGIN" << Tuple<double>(X0,3) << endl
      << "SPACING" << Tuple<double>(dX,3) << endl
      << endl;
    #endif

    set<string>::iterator it;
    set<string>::iterator begin=this->InputArrays.begin();
    set<string>::iterator end=this->InputArrays.end();
    for (it=begin; it!=end; ++it)
      {
      vtkDataArray *V=inImData->GetPointData()->GetArray((*it).c_str());
      if (V==0)
        {
        vtkErrorMacro(
          << "Array " << (*it).c_str()
          << " was requested but is not present");
        continue;
        }

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

      if (this->EnableCUDA)
        {
        #ifdef vtkSQKernelConvolutionDEBUG
        pCerr() << "using the GPU" << endl;
        #endif
        this->CUDADriver->Convolution(
            inputExt,
            outputExt,
            this->KernelExt,
            nGhost,
            this->Mode,
            V,
            W,
            this->Kernel);
        }
      else
        {
        #ifdef vtkSQKernelConvolutionDEBUG
        pCerr() << "using the CPU" << endl;
        #endif
        this->CPUDriver->Convolution(
            inputExt,
            outputExt,
            this->KernelExt,
            nGhost,
            this->Mode,
            V,
            W,
            this->Kernel);
        }

      outImData->GetPointData()->AddArray(W);
      W->Delete();
      }

    // outImData->Print(cerr);
    }
  else
  if (isRecti)
    {
    vtkWarningMacro("TODO : implment difference opperators on stretched grids.");
    }

  #if defined vtkSQKernelConvolutionTIME
  log->EndEvent("vtkSQKernelConvolution::RequestData");
  #endif

 return 1;
}

//-----------------------------------------------------------------------------
void vtkSQKernelConvolution::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQKernelConvolutionDEBUG
  pCerr() << "=====vtkSQKernelConvolution::PrintSelf" << endl;
  #endif

  this->Superclass::PrintSelf(os,indent);

  // TODO

}
