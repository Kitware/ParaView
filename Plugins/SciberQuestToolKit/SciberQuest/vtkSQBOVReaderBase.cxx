/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBOVReaderBase.h"

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
  // only usefull in terminals
  #undef SQTK_DEBUG
#endif

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQBOVReaderBase);

//-----------------------------------------------------------------------------
vtkSQBOVReaderBase::vtkSQBOVReaderBase()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::vtkSQBOVReaderBase" << std::endl;
  #endif

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Initialize variables
  this->FileName=0;
  this->FileNameChanged=false;
  this->Subset[0]=
  this->Subset[1]=
  this->Subset[2]=
  this->Subset[3]=
  this->Subset[4]=
  this->Subset[5]=0;
  this->ISubsetRange[0]=1;
  this->ISubsetRange[1]=0;
  this->JSubsetRange[0]=1;
  this->JSubsetRange[1]=0;
  this->KSubsetRange[0]=1;
  this->KSubsetRange[1]=0;
  this->UseCollectiveIO=HINT_DISABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDirectIO=HINT_AUTOMATIC;
  this->UseDeferredOpen=HINT_DEFAULT;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->WorldRank=0;
  this->WorldSize=1;
  this->LogLevel=0;

  // this reader requires MPI, both the build and the runtime
  // but we won't report the error here because pvclient may
  // construct the reader to check if it could read a givenn file
  // the error should reported in SetFileName.
  #ifndef SQTK_WITHOUT_MPI
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (mpiOk)
    {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
    }
  #endif

  // Configure the internal reader.
  this->Reader=BOVReader::New();

  GDAMetaData md;
  this->Reader->SetMetaData(&md);
}

//-----------------------------------------------------------------------------
vtkSQBOVReaderBase::~vtkSQBOVReaderBase()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::~vtkSQBOVReaderBase" << std::endl;
  #endif

  this->Clear();
  this->Reader->Delete();
  this->Reader=0;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::Clear()
{
  this->SetFileName(0);
  this->FileNameChanged=false;
  this->Subset[0]=
  this->Subset[1]=
  this->Subset[2]=
  this->Subset[3]=
  this->Subset[4]=
  this->Subset[5]=0;
  this->ISubsetRange[0]=1;
  this->ISubsetRange[1]=0;
  this->JSubsetRange[0]=1;
  this->JSubsetRange[1]=0;
  this->KSubsetRange[0]=1;
  this->KSubsetRange[1]=0;
  this->UseCollectiveIO=HINT_ENABLED;
  this->NumberOfIONodes=0;
  this->UseDirectIO=HINT_AUTOMATIC;
  this->CollectBufferSize=0;
  this->UseDeferredOpen=HINT_DEFAULT;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->Reader->Close();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::Initialize(
      vtkPVXMLElement *root,
      const char *fileName,
      std::vector<std::string> &arrays)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::Initialize" << std::endl;
  #endif

  vtkPVXMLElement *elem=(vtkPVXMLElement*)0;

  if ( ! ( ((elem=GetOptionalElement(root,"vtkSQBOVReader"))!=0)
        || ((elem=GetOptionalElement(root,"vtkSQBOVMetaReader"))!=0) ) )
    {
    return -1;
    }

  // hints
  int cb_enable=0;
  GetOptionalAttribute<int,1>(elem,"cb_enable",&cb_enable);
  if (cb_enable==0)
    {
    this->SetUseCollectiveIO(vtkSQBOVReaderBase::HINT_DISABLED);
    }
  else
  if (cb_enable>0)
    {
    this->SetUseCollectiveIO(vtkSQBOVReaderBase::HINT_ENABLED);
    }

  int cb_buffer_size=0;
  GetOptionalAttribute<int,1>(elem,"cb_buffer_size",&cb_buffer_size);
  if (cb_buffer_size)
    {
    this->SetCollectBufferSize(cb_buffer_size);
    }
  // open the file, do it now to get domain/subset info
  this->SetFileName(fileName);
  if (!this->IsOpen())
    {
    sqErrorMacro(pCerr(),"Failed to open " << fileName);
    return -1;
    }

  this->SetUseDirectIO(HINT_DEFAULT);
  int direct_io=-1;
  GetOptionalAttribute<int,1>(elem,"direct_io",&direct_io);
  if (direct_io==0)
    {
    this->SetUseDirectIO(HINT_DISABLED);
    }
  else
  if (direct_io==1)
    {
    this->SetUseDirectIO(HINT_ENABLED);
    }

  // subset the data
  // when the user passes -1, we'll use the whole extent
  int wholeExtent[6];
  this->GetSubset(wholeExtent);
  int subset[6]={-1,-1,-1,-1,-1,-1};
  GetOptionalAttribute<int,2>(elem,"ISubset",subset);
  GetOptionalAttribute<int,2>(elem,"JSubset",subset+2);
  GetOptionalAttribute<int,2>(elem,"KSubset",subset+4);
  for (int i=0; i<6; ++i)
    {
    if (subset[i]<0) subset[i]=wholeExtent[i];
    }
  this->SetSubset(subset);

  // return selected arrays
  // when none are selected we return all available
  size_t nArrays=0;
  elem=GetOptionalElement(elem,"arrays");
  if (elem)
    {
    ExtractValues(elem->GetCharacterData(),arrays);
    nArrays=arrays.size();
    if (nArrays<1)
      {
      sqErrorMacro(pCerr(),"Error: parsing <arrays>.");
      return -1;
      }
    }
  else
    {
    nArrays=this->GetNumberOfPointArrays();
    for (size_t i=0; i<nArrays; ++i)
      {
      const char *arrayName=this->GetPointArrayName((int)i);
      arrays.push_back(arrayName);
      }
    }

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  int globalLogLevel=log->GetGlobalLevel();
  if (this->LogLevel || globalLogLevel)
    {
    log->GetHeader()
      << "# ::vtkSQBOVReaderBase" << "\n"
      << "#   cb_enable=" << this->GetUseCollectiveIO() << "\n"
      << "#   cb_buffer_size=" << this->GetCollectBufferSize() << "\n"
      << "#   wholeExtent=" << Tuple<int>(wholeExtent,6) << "\n"
      << "#   subsetExtent=" << Tuple<int>(subset,6) << "\n"
      << "#   arrays=";
    for (size_t i=0; i<nArrays; ++i)
      {
      *log << " " << arrays[i];
      }
    log->GetHeader() << "\n";
    }

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::CanReadFile(const char *file)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::CanReadFile" << std::endl;
  pCerr() << "Check " << safeio(file) << "." << std::endl;
  #endif

  int status=0;

  #ifdef SQTK_WITHOUT_MPI
  (void)file;
  #else
  // first check that MPI is initialized. in builtin mode MPI will
  // never be initialized and this reader will be unable to read files
  // so we always return false in this case
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    return 0;
    }

  // only rank 0 opens the file, this results in metadata
  // being parsed. If the parsing of md is successful then
  // the file is ours.
  this->Reader->SetCommunicator(MPI_COMM_SELF);
  status=this->Reader->Open(file);
  this->Reader->Close();
  #endif

  return status;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetFileName(const char* _arg)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::SetFileName" << std::endl;
  pCerr() << "Set FileName " << safeio(_arg) << "." << std::endl;
  #endif

  #ifdef SQTK_WITHOUT_MPI
  (void)_arg;
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (_arg && !mpiOk)
    {
    vtkErrorMacro(
        << "MPI has not been initialized. Restart ParaView using mpiexec.");
    return;
    }

  if (this->FileName == NULL && _arg == NULL) { return;}
  if (this->FileName && _arg && (!strcmp(this->FileName,_arg))) { return;}
  if (this->FileName) { delete [] this->FileName; }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->FileName = cp1;
    do { *cp1++ = *cp2++; } while ( --n );
    }
  else
    {
    this->FileName = NULL;
    }

  // Close the currently opened dataset (if any).
  if (this->Reader->IsOpen())
    {
    this->Reader->Close();
    }

  // Open the newly named dataset.
  if (this->FileName)
    {
    vtkSQLog *log=vtkSQLog::GetGlobalInstance();
    int globalLogLevel=log->GetGlobalLevel();
    if (this->LogLevel || globalLogLevel)
      {
      log->StartEvent("vtkSQBOVReaderBase::Open");
      }

    this->Reader->SetCommunicator(MPI_COMM_WORLD);
    int ok=this->Reader->Open(this->FileName);

    if (this->LogLevel || globalLogLevel)
      {
      log->EndEvent("vtkSQBOVReaderBase::Open");
      }

    if(!ok)
      {
      vtkErrorMacro("Failed to open the file \"" << safeio(this->FileName) << "\".");
      return;
      }

    // Update index space ranges provided in the U.I.
    const CartesianExtent &subset=this->Reader->GetMetaData()->GetSubset();
    this->ISubsetRange[0]=this->Subset[0]=subset[0];
    this->ISubsetRange[1]=this->Subset[1]=subset[1];
    this->JSubsetRange[0]=this->Subset[2]=subset[2];
    this->JSubsetRange[1]=this->Subset[3]=subset[3];
    this->KSubsetRange[0]=this->Subset[4]=subset[4];
    this->KSubsetRange[1]=this->Subset[5]=subset[5];

    #if defined SQTK_DEBUG
    pCerr()
      << "vtkSQBOVReaderBase "
      << this->WorldRank
      << " Open succeeded." << std::endl;
    #endif
    }

  this->Modified();
  #endif
}

//-----------------------------------------------------------------------------
bool vtkSQBOVReaderBase::IsOpen()
{
  return this->Reader->IsOpen();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetSubset(const int *s)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::SetSubset*" << std::endl;
  #endif
  this->SetSubset(s[0],s[1],s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetSubset(int ilo,int ihi, int jlo, int jhi, int klo, int khi)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::SetSubset" << std::endl;
  #endif
  // Avoid unecessary pipeline execution.
  if ( this->Subset[0]==ilo && this->Subset[1]==ihi
    && this->Subset[2]==jlo && this->Subset[3]==jhi
    && this->Subset[4]==klo && this->Subset[5]==khi )
    {
    return;
    }

  // Copy for get pointer return.
  this->Subset[0]=ilo;
  this->Subset[1]=ihi;
  this->Subset[2]=jlo;
  this->Subset[3]=jhi;
  this->Subset[4]=klo;
  this->Subset[5]=khi;

  // Pass through to reader.
  CartesianExtent subset(this->Subset);
  this->Reader->GetMetaData()->SetSubset(subset);

  // Mark object dirty.
  this->Modified();

  #if defined SQTK_DEBUG
  pCerr() << "SetSubset(" << subset << ")" << std::endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetISubset(int ilo, int ihi)
{
  int *s=this->Subset;
  this->SetSubset(ilo,ihi,s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetJSubset(int jlo, int jhi)
{
  int *s=this->Subset;
  this->SetSubset(s[0],s[1],jlo,jhi,s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetKSubset(int klo, int khi)
{
  int *s=this->Subset;
  this->SetSubset(s[0],s[1],s[2],s[3],klo,khi);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetVectorProjection(int mode)
{
  this->Reader->SetVectorProjection(mode);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::GetVectorProjection()
{
  return this->Reader->GetVectorProjection();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::GetNumberOfTimeSteps()
{
  return (int)this->Reader->GetMetaData()->GetNumberOfTimeSteps();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::GetTimeSteps(double *times)
{
  size_t n=this->Reader->GetMetaData()->GetNumberOfTimeSteps();
  double dt=this->Reader->GetMetaData()->GetDt();

  for (size_t i=0; i<n; ++i)
    {
    times[i]=dt*this->Reader->GetMetaData()->GetTimeStep(i);
    }
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetPointArrayStatus(const char *name, int status)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::SetPointArrayStatus" << std::endl;
  pCerr() << safeio(name) << " " << status << std::endl;
  #endif
  if (status)
    {
    this->Reader->GetMetaData()->ActivateArray(name);
    }
  else
    {
    this->Reader->GetMetaData()->DeactivateArray(name);
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::GetPointArrayStatus(const char *name)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::GetPointArrayStatus" << std::endl;
  #endif
  return this->Reader->GetMetaData()->IsArrayActive(name);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::GetNumberOfPointArrays()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::GetNumberOfPointArrays" << std::endl;
  #endif
  return (int)this->Reader->GetMetaData()->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSQBOVReaderBase::GetPointArrayName(int idx)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::GetArrayName" << std::endl;
  #endif
  return this->Reader->GetMetaData()->GetArrayName(idx);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::ClearPointArrayStatus()
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::ClearPointArrayStatus" << std::endl;
  #endif

  int nArrays=this->GetNumberOfPointArrays();
  for (int i=0; i<nArrays; ++i)
    {
    const char *aName=this->GetPointArrayName(i);
    this->SetPointArrayStatus(aName,0);
    }
}

//----------------------------------------------------------------------------
int vtkSQBOVReaderBase::RequestDataObject(
      vtkInformation* /*req*/,
      vtkInformationVector** /*inInfos*/,
      vtkInformationVector* outInfos)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReaderBase::RequestDataObject" << std::endl;
  #endif

  vtkInformation* info=outInfos->GetInformationObject(0);

  vtkDataObject *dataset=this->Reader->GetDataSet();

  info->Set(vtkDataObject::DATA_TYPE_NAME(),this->Reader->GetDataSetType());
  info->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_3D_EXTENT);
  info->Set(vtkDataObject::DATA_OBJECT(),dataset);

  dataset->Delete();

  #if defined SQTK_DEBUG
  pCerr() << "datasetType=" << info->Get(vtkDataObject::DATA_TYPE_NAME()) << std::endl;
  pCerr() << "dataset=" << info->Get(vtkDataObject::DATA_OBJECT()) << std::endl;
  pCerr() << "info="; info->Print(std::cerr);
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::RequestInformation(
  vtkInformation *req,
  vtkInformationVector **inInfos,
  vtkInformationVector* outInfos)
{
  #if defined SQTK_DEBUG
  pCerr() << "=====vtkSQBOVReader::RequestInformationImage" << std::endl;
  #endif

  (void)req;
  (void)inInfos;

  if (!this->Reader->IsOpen())
    {
    vtkWarningMacro("No file open, cannot process RequestInformation!");
    return 1;
    }

  // Determine which time steps are available.
  size_t nSteps=this->Reader->GetMetaData()->GetNumberOfTimeSteps();
  const int *steps=this->Reader->GetMetaData()->GetTimeSteps();
  double dt=this->Reader->GetMetaData()->GetDt();
  std::vector<double> times(nSteps,0.0);
  for (size_t i=0; i<nSteps; ++i)
    {
    times[i]=dt*static_cast<double>(steps[i]);
    }

  #if defined SQTK_DEBUG
  pCerr() << times << std::endl;
  pCerr() << "Total: " << nSteps << std::endl;
  #endif

  // Set available time steps on pipeline.
  vtkInformation *info=outInfos->GetInformationObject(0);

  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_STEPS(),
    &times[0],
    (int)times.size());

  double timeRange[2]={times[0],times[times.size()-1]};
  info->Set(
    vtkStreamingDemandDrivenPipeline::TIME_RANGE(),
    timeRange,
    2);

  return 1;
}


//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::SetMPIFileHints()
{
  #ifndef SQTK_WITHOUT_MPI
  MPI_Info hints;
  MPI_Info_create(&hints);

  switch (this->UseCollectiveIO)
    {
    case HINT_AUTOMATIC:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_cb_read","disable");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_cb_read","enable");
      break;
    default:
      vtkErrorMacro("Invalid value for UseCollectiveIO.");
      break;
    }

  if (this->NumberOfIONodes>0)
    {
    std::ostringstream os;
    os << this->NumberOfIONodes;
    MPI_Info_set(hints,"cb_nodes",const_cast<char *>(os.str().c_str()));
    }

  if (this->CollectBufferSize>0)
    {
    std::ostringstream os;
    os << this->CollectBufferSize;
    MPI_Info_set(hints,"cb_buffer_size",const_cast<char *>(os.str().c_str()));
    }

  switch (this->UseDirectIO)
    {
    case HINT_DEFAULT:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"direct_write","false");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"direct_write","true");
      break;
    default:
      vtkErrorMacro("Invalid value for UseDirectIO.");
      break;
    }

  switch (this->UseDeferredOpen)
    {
    case HINT_DEFAULT:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_no_indep_rw","false");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_no_indep_rw","true");
      break;
    default:
      vtkErrorMacro("Invalid value for UseDeferredOpen.");
      break;
    }

  switch (this->UseDataSieving)
    {
    case HINT_AUTOMATIC:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_ds_read","disable");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_ds_read","enable");
      break;
    default:
      vtkErrorMacro("Invalid value for UseDataSieving.");
      break;
    }

  if (this->SieveBufferSize>0)
    {
    std::ostringstream os;
    os << this->SieveBufferSize;
    MPI_Info_set(hints,"ind_rd_buffer_size", const_cast<char *>(os.str().c_str()));
    }

  this->Reader->SetHints(hints);

  MPI_Info_free(&hints);
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQBOVReaderBase::GetTimeStepId(
        vtkInformation *inInfo,
        vtkInformation *outInfo)
{
  // Figure out which of our time steps is closest to the requested
  // one, fallback to the 0th step.
  int stepId=this->Reader->GetMetaData()->GetTimeStep(0);
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    double step
      = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int nSteps
      = inInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* steps =
      inInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    for (int i=0; i<nSteps; ++i)
      {
      if (fequal(steps[i],step,1E-10))
        {
        stepId=this->Reader->GetMetaData()->GetTimeStep(i);
        break;
        }
      }

    inInfo->Set(vtkDataObject::DATA_TIME_STEP(),step);
    outInfo->Set(vtkDataObject::DATA_TIME_STEP(),step);

    #if defined SQTK_DEBUG
    pCerr() << "Requested time " << step << " using " << stepId << "." << std::endl;
    #endif
    }

  return stepId;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReaderBase::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName:        " << safeio(this->FileName) << std::endl;
  os << indent << "FileNameChanged: " << this->FileNameChanged << std::endl;
  os << indent << "Raeder: " << std::endl;
  this->Reader->PrintSelf(os);
  os << std::endl;
}
