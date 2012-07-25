/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBOVReader.h"

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
using std::min;
using std::max;

#include <sstream>
using std::ostringstream;

// #define vtkSQBOVReaderDEBUG
// #define vtkSQBOVReaderTIME

#ifdef WIN32
  // for gethostname on windows.
  #include <Winsock2.h>
  // these are only usefull in terminals
  #undef vtkSQBOVReaderTIME
  #undef vtkSQBOVReaderDEBUG
#endif

#if defined vtkSQBOVReaderTIME
  #include "vtkSQLog.h"
#endif

#ifndef HOST_NAME_MAX
  #define HOST_NAME_MAX 255
#endif

// disbale warning about passing string literals.
#if !defined(__INTEL_COMPILER) && defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

vtkStandardNewMacro(vtkSQBOVReader);

//-----------------------------------------------------------------------------
vtkSQBOVReader::vtkSQBOVReader()
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================vtkSQBOVReader" << endl;
  #endif

  // Initialize pipeline.
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);

  // Initialize variables
  this->MetaRead=0;
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
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->NGhosts=1;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->UseCollectiveIO=HINT_DISABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDeferredOpen=HINT_DEFAULT;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->WorldRank=0;
  this->WorldSize=1;

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

  this->HostName[0]='\0';
  #if defined vtkSQBOVReaderDEBUG
  char hostname[HOST_NAME_MAX];
  gethostname(hostname,HOST_NAME_MAX);
  hostname[4]='\0';
  for (int i=0; i<5; ++i)
    {
    this->HostName[i]=hostname[i];
    }
  #endif

  // Configure the internal reader.
  this->Reader=BOVReader::New();

  GDAMetaData md;
  this->Reader->SetMetaData(&md);
}

//-----------------------------------------------------------------------------
vtkSQBOVReader::~vtkSQBOVReader()
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================~vtkSQBOVReader" << endl;
  #endif

  this->Clear();
  this->Reader->Delete();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::Clear()
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
  this->PeriodicBC[0]=
  this->PeriodicBC[1]=
  this->PeriodicBC[2]=0;
  this->NGhosts=1;
  this->DecompDims[0]=
  this->DecompDims[1]=
  this->DecompDims[2]=1;
  this->BlockCacheSize=10;
  this->ClearCachedBlocks=1;
  this->UseCollectiveIO=HINT_ENABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDeferredOpen=HINT_DEFAULT;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->Reader->Close();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::Initialize(
      vtkPVXMLElement *root,
      const char *fileName,
      vector<string> &arrays)
{
  vtkPVXMLElement *elem=GetOptionalElement(root,"vtkSQBOVReader");
  if (elem==0)
    {
    //sqErrorMacro(pCerr(),"Element for vtkSQBOVReader not present.");
    return -1;
    }

  // hints
  this->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);
  this->SetUseCollectiveIO(vtkSQBOVReader::HINT_AUTOMATIC);
  int cb_enable=0;
  GetOptionalAttribute<int,1>(elem,"cb_enable",&cb_enable);
  if (cb_enable==0)
    {
    this->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
    }
  else
  if (cb_enable>0)
    {
    this->SetUseCollectiveIO(vtkSQBOVReader::HINT_ENABLED);
    }

  int cb_buffer_size=0;
  GetOptionalAttribute<int,1>(elem,"cb_buffer_size",&cb_buffer_size);
  if (cb_buffer_size)
    {
    this->SetCollectBufferSize(cb_buffer_size);
    }

  // meta-read
  int meta_read=0;
  GetOptionalAttribute<int,1>(elem,"meta_read",&meta_read);
  if (meta_read>0)
    {
    this->SetMetaRead(1);

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

    this->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
    }

  // open the file, do it now to get domain/subset info
  this->SetFileName(fileName);
  if (!this->IsOpen())
    {
    sqErrorMacro(pCerr(),"Failed to open " << fileName);
    return -1;
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

  #if defined vtkSQBOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQBOVReader" << "\n"
    << "#   cb_enable=" << cb_enable << "\n"
    << "#   cb_buffer_size=" << cb_buffer_size << "\n"
    << "#   wholeExtent=" << Tuple<int>(wholeExtent,6) << "\n"
    << "#   subsetExtent=" << Tuple<int>(subset,6) << "\n";
  for (size_t i=0; i<nArrays; ++i)
    {
    *log << "#   arrayName_" << i << "=" << arrays[i] << "\n";
    }
  *log << "\n";
  #endif

  return 0;
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::CanReadFile(const char *file)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================CanReadFile" << endl;
  pCerr() << "Check " << safeio(file) << "." << endl;
  #endif

  int status=0;

  #ifdef SQTK_WITHOUT_MPI
  (void)file;
  #else
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
void vtkSQBOVReader::SetFileName(const char* _arg)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================SetFileName" << endl;
  pCerr() << "Set FileName " << safeio(_arg) << "." << endl;
  #endif
  #if defined vtkSQBOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQBOVReader::SetFileName");
  #endif

  #ifdef SQTK_WITHOUT_MPI
  (void)_arg;
  //vtkErrorMacro(
  //    << "This class requires MPI however it was built without MPI.");
  #else
  int mpiOk=0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
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
    this->Reader->SetCommunicator(MPI_COMM_WORLD);
    if(!this->Reader->Open(this->FileName))
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

    #if defined vtkSQBOVReaderDEBUG
    pCerr()
      << "vtkSQBOVReader "
      << this->HostName << " "
      << this->WorldRank
      << " Open succeeded." << endl;
    #endif
    }

  this->Modified();
  #endif

  #if defined vtkSQBOVReaderTIME
  log->EndEvent("vtkSQBOVReader::SetFileName");
  #endif
}

//-----------------------------------------------------------------------------
bool vtkSQBOVReader::IsOpen()
{
  return this->Reader->IsOpen();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetSubset(const int *s)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================SetSubset*" << endl;
  #endif
  this->SetSubset(s[0],s[1],s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetSubset(int ilo,int ihi, int jlo, int jhi, int klo, int khi)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================SetSubset" << endl;
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

  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "SetSubset(" << subset << ")" << endl;
  #endif
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetISubset(int ilo, int ihi)
{
  int *s=this->Subset;
  this->SetSubset(ilo,ihi,s[2],s[3],s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetJSubset(int jlo, int jhi)
{
  int *s=this->Subset;
  this->SetSubset(s[0],s[1],jlo,jhi,s[4],s[5]);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetKSubset(int klo, int khi)
{
  int *s=this->Subset;
  this->SetSubset(s[0],s[1],s[2],s[3],klo,khi);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetVectorProjection(int mode)
{
  this->Reader->SetVectorProjection(mode);
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::GetVectorProjection()
{
  return this->Reader->GetVectorProjection();
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::GetNumberOfTimeSteps()
{
  return (int)this->Reader->GetMetaData()->GetNumberOfTimeSteps();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::GetTimeSteps(double *times)
{
  size_t n=this->Reader->GetMetaData()->GetNumberOfTimeSteps();

  for (size_t i=0; i<n; ++i)
    {
    times[i]=this->Reader->GetMetaData()->GetTimeStep(i);
    }
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetPeriodicBC(int *flags)
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
void vtkSQBOVReader::SetXHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[0]==flag)
    {
    return;
    }

  this->PeriodicBC[0]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetYHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[1]==flag)
    {
    return;
    }

  this->PeriodicBC[1]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetZHasPeriodicBC(int flag)
{
  if (this->PeriodicBC[2]==flag)
    {
    return;
    }

  this->PeriodicBC[2]=flag;

  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::SetPointArrayStatus(const char *name, int status)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================SetPointArrayStatus" << endl;
  pCerr() << safeio(name) << " " << status << endl;
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
int vtkSQBOVReader::GetPointArrayStatus(const char *name)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================GetPointArrayStatus" << endl;
  #endif
  return this->Reader->GetMetaData()->IsArrayActive(name);
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::GetNumberOfPointArrays()
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================GetNumberOfPointArrays" << endl;
  #endif
  return (int)this->Reader->GetMetaData()->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSQBOVReader::GetPointArrayName(int idx)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================GetArrayName" << endl;
  #endif
  return this->Reader->GetMetaData()->GetArrayName(idx);
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::ClearPointArrayStatus()
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================ClearPointArrayStatus" << endl;
  #endif

  int nArrays=this->GetNumberOfPointArrays();
  for (int i=0; i<nArrays; ++i)
    {
    const char *aName=this->GetPointArrayName(i);
    this->SetPointArrayStatus(aName,0);
    }
}

//----------------------------------------------------------------------------
int vtkSQBOVReader::RequestDataObject(
      vtkInformation* /*req*/,
      vtkInformationVector** /*inInfos*/,
      vtkInformationVector* outInfos)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================RequestDataObject" << endl;
  #endif

  vtkInformation* info=outInfos->GetInformationObject(0);

  vtkDataObject *dataset=this->Reader->GetDataSet();

  info->Set(vtkDataObject::DATA_TYPE_NAME(),this->Reader->GetDataSetType());
  info->Set(vtkDataObject::DATA_EXTENT_TYPE(),VTK_3D_EXTENT);
  info->Set(vtkDataObject::DATA_OBJECT(),dataset);

  //dataset->SetPipelineInformation(info);
  dataset->Delete();

  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "datasetType=" << info->Get(vtkDataObject::DATA_TYPE_NAME()) << endl;
  pCerr() << "dataset=" << info->Get(vtkDataObject::DATA_OBJECT()) << endl;
  pCerr() << "info="; info->Print(cerr);
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::RequestInformation(
  vtkInformation*,
  vtkInformationVector**,
  vtkInformationVector* outInfos)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================RequestInformationImage" << endl;
  #endif

  if (!this->Reader->IsOpen())
    {
    vtkWarningMacro("No file open, cannot process RequestInformation!");
    return 1;
    }

  vtkInformation *info=outInfos->GetInformationObject(0);

  // The two modes to run the reader are Meta mode and Actual mode.
  // In meta mode no data is read rather the pipeline is tricked into
  // ploting a bounding box matching the data size. Keys are provided
  // so that actual read may take place downstream. In actual mode
  // the reader reads the requested arrays.
  if (this->MetaRead)
    {
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
    info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),wholeExtent,6);
    // req->Append(
    //     vtkExecutive::KEYS_TO_COPY(),
    //     vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

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
    }
  else
    {
    // The actual read we need to populate the VTK keys with actual
    // values. The mechanics of the pipeline require that the data set
    // dimensions and whole extent key reflect the global index space
    // of the dataset, the data set extent will have the decomposed
    // index space.
    int wholeExtent[6];
    this->GetSubset(wholeExtent);
    info->Set(
      vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
      wholeExtent,
      6);
    // req->Append(
    //     vtkExecutive::KEYS_TO_COPY(),
    //     vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

    #if defined vtkSQBOVReaderDEBUG
    pCerr() << "WHOLE_EXTENT=" << Tuple<int>(wholeExtent,6) << endl;
    #endif

    if (this->Reader->DataSetTypeIsImage())
      {
      double X0[3];
      this->Reader->GetMetaData()->GetOrigin(X0);
      info->Set(vtkDataObject::ORIGIN(),X0,3);
      // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::ORIGIN());

      double dX[3];
      this->Reader->GetMetaData()->GetSpacing(dX);
      info->Set(vtkDataObject::SPACING(),dX,3);
      // req->Append(vtkExecutive::KEYS_TO_COPY(),vtkDataObject::SPACING());

      #if defined vtkSQBOVReaderDEBUG
      pCerr() << "ORIGIN=" << Tuple<double>(X0,3) << endl;
      pCerr() << "SPACING=" << Tuple<double>(dX,3) << endl;
      #endif
      }
    }

  // Determine which time steps are available.
  size_t nSteps=this->Reader->GetMetaData()->GetNumberOfTimeSteps();
  const int *steps=this->Reader->GetMetaData()->GetTimeSteps();
  vector<double> times(nSteps,0.0);
  for (size_t i=0; i<nSteps; ++i)
    {
    times[i]=(double)steps[i]; // use the index rather than the actual.
    }

  #if defined vtkSQBOVReaderDEBUG
  pCerr() << times << endl;
  pCerr() << "Total: " << nSteps << endl;
  #endif

  // Set available time steps on pipeline.
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
void vtkSQBOVReader::SetMPIFileHints()
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
    ostringstream os;
    os << this->NumberOfIONodes;
    MPI_Info_set(hints,"cb_nodes",const_cast<char *>(os.str().c_str()));
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

  if (this->CollectBufferSize>0)
    {
    ostringstream os;
    os << this->CollectBufferSize;
    MPI_Info_set(hints,"cb_buffer_size",const_cast<char *>(os.str().c_str()));
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
    ostringstream os;
    os << this->SieveBufferSize;
    MPI_Info_set(hints,"ind_rd_buffer_size", const_cast<char *>(os.str().c_str()));
    }

  this->Reader->SetHints(hints);

  MPI_Info_free(&hints);
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQBOVReader::RequestData(
        vtkInformation *req,
        vtkInformationVector ** /*input*/,
        vtkInformationVector *outInfos)
{
  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "===============================RequestData" << endl;
  #endif
  #if defined vtkSQBOVReaderTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQBOVReader::RequestData");
  #endif

  vtkInformation *info=outInfos->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *output
    = dynamic_cast<vtkDataSet *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (output==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  // Figure out which of our time steps is closest to the requested
  // one, fallback to the 0th step.
  int stepId=this->Reader->GetMetaData()->GetTimeStep(0);
  if (info->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
    {
    double step
      = info->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int nSteps
      = info->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
    double* steps =
      info->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS());

    for (int i=0; i<nSteps; ++i)
      {
      if (fequal(steps[i],step,1E-10))
        {
        stepId=this->Reader->GetMetaData()->GetTimeStep(i);
        break;
        }
      }
    info->Set(vtkDataObject::DATA_TIME_STEP(),step);
    output->GetInformation()->Set(vtkDataObject::DATA_TIME_STEP(),step);
    #if defined vtkSQBOVReaderDEBUG
    pCerr() << "Requested time " << step << " using " << stepId << "." << endl;
    #endif
    }

  BOVMetaData *md=this->Reader->GetMetaData();

  // The subset is the what the user selected in the GUI. This is what will
  // be loaded in aggregate across the entire run.
  CartesianExtent subset=md->GetSubset();

  #if defined vtkSQBOVReaderDEBUG
  if (this->WorldRank==0)
    {
    pCerr()
      << "subset=" << subset
      << " size=" << subset.Size()*sizeof(float)
      << endl;
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

  #if defined vtkSQBOVReaderDEBUG
  pCerr() << "decomp=" << decomp << endl;
  #endif

  // Construct MPI File hints for the reader.
  this->SetMPIFileHints();

  // The two modes to run the reader are Meta mode and Actual mode.
  // In meta mode no data is read here rather the ParaView is tricked into
  // ploting a bounding box matching the data size. Keys are provided
  // so that actual read may take place downstream. In actual mode
  // the reader reads the requested arrays here and places them in the
  // output as usual.

  if (this->MetaRead)
    {
    /// Meta Mode

    // pass the boundary condition flags
    info->Set(vtkSQOOCReader::PERIODIC_BC(),this->PeriodicBC,3);
    req->Append(vtkExecutive::KEYS_TO_COPY(), vtkSQOOCReader::PERIODIC_BC());

    CartesianDecomp *ddecomp;

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

    // This reader can read vtkImageData, vtkRectilinearGrid, and vtkStructuredData

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
      info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),subsetBounds,6);
      req->Append(
          vtkExecutive::KEYS_TO_COPY(),
          vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());

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
      info->Set(vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),subsetBounds,6);
      req->Append(
          vtkExecutive::KEYS_TO_COPY(),
          vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());

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
      vtkErrorMacro(
        << "Read failed." << endl << *md);
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
    info->Set(vtkSQOOCReader::READER(),OOCReader);
    OOCReader->Delete();
    ddecomp->Delete();
    req->Append(vtkExecutive::KEYS_TO_COPY(),vtkSQOOCReader::READER());
    }
  else
    {
    /// Actual Mode

    // A read pulls data from file on a cell centered grid into memory
    // on a node centered grid. file->nCells==memory->nPoints

    // Set the timestep to be read.
    BOVTimeStepImage *stepImg=this->Reader->OpenTimeStep(stepId);

    if (this->Reader->DataSetTypeIsImage())
      {
      // read onto a uniform grid

      // Pull origin and spacing we stored during RequestInformation pass.
      double dX[3];
      info->Get(vtkDataObject::SPACING(),dX);
      double X0[3];
      info->Get(vtkDataObject::ORIGIN(),X0);

      int nPoints[3];
      subset.Size(nPoints);

      // Configure the output.
      vtkImageData *idds=dynamic_cast<vtkImageData*>(output);
      idds->SetDimensions(nPoints);
      idds->SetOrigin(X0);
      idds->SetSpacing(dX);
      idds->SetExtent(decomp.GetData());

      // Store the bounds of the aggregate dataset.
      double subsetBounds[6];
      CartesianExtent::GetBounds(
            subset,
            X0,
            dX,
            CartesianExtent::DIM_MODE_3D,
            subsetBounds);

      info->Set(
          vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
          subsetBounds,
          6);
      req->Append(
          vtkExecutive::KEYS_TO_COPY(),
          vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());
      }
    else
    if (this->Reader->DataSetTypeIsRectilinear())
      {
      // read onto the stretched grid
      int nPoints[3];
      decomp.Size(nPoints);

      // configure the output
      vtkRectilinearGrid *rgds=dynamic_cast<vtkRectilinearGrid*>(output);
      rgds->SetExtent(decomp.GetData());

      vtkFloatArray *fa;
      fa=vtkFloatArray::New();
      fa->SetArray(md->SubsetCoordinate(0,decomp),nPoints[0],0);
      rgds->SetXCoordinates(fa);
      fa->Delete();

      fa=vtkFloatArray::New();
      fa->SetArray(md->SubsetCoordinate(1,decomp),nPoints[1],0);
      rgds->SetYCoordinates(fa);
      fa->Delete();

      fa=vtkFloatArray::New();
      fa->SetArray(md->SubsetCoordinate(2,decomp),nPoints[2],0);
      rgds->SetZCoordinates(fa);
      fa->Delete();

      // Store the bounds of the aggregate dataset.
      double subsetBounds[6];
      CartesianExtent::GetBounds(
          subset,
          md->GetCoordinate(0)->GetPointer(),
          md->GetCoordinate(1)->GetPointer(),
          md->GetCoordinate(2)->GetPointer(),
          CartesianExtent::DIM_MODE_3D,
          subsetBounds);

      info->Set(
          vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX(),
          subsetBounds,
          6);
      req->Append(
          vtkExecutive::KEYS_TO_COPY(),
          vtkStreamingDemandDrivenPipeline::WHOLE_BOUNDING_BOX());
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
      // unrecognized dataset type
      vtkErrorMacro(
        << "Error: invalid dataset type \""
        << md->GetDataSetType() << "\".");
      }

    // Read the selected arrays into the output.
    int ok=this->Reader->ReadTimeStep(stepImg,output,this);
    this->Reader->CloseTimeStep(stepImg);
    if (!ok)
      {
      vtkErrorMacro(
        << "Read failed." << endl << *md);
      output->Initialize();
      return 1;
      }
    }

  // Give implementation classes a chance to store specialized keys
  // into the pipeline.
  md->PushPipelineInformation(req, info);

  #if defined vtkSQBOVReaderDEBUG
  this->Reader->PrintSelf(pCerr());
  output->Print(pCerr());
  #endif

  #if defined vtkSQBOVReaderTIME
  log->EndEvent("vtkSQBOVReader::RequestData");
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
void vtkSQBOVReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName:        " << safeio(this->FileName) << endl;
  os << indent << "FileNameChanged: " << this->FileNameChanged << endl;
  os << indent << "Raeder: " << endl;
  this->Reader->PrintSelf(os);
  os << endl;
}

/// PV U.I.
// //----------------------------------------------------------------------------
// // observe PV interface and set modified if user makes changes
// void vtkSQBOVReader::SelectionModifiedCallback(
//         vtkObject*,
//         unsigned long,
//         void* clientdata,
//         void*)
// {
//   vtkSQBOVReader *dbb
//     = static_cast<vtkSQBOVReader*>(clientdata);
//
//   dbb->Modified();
// }
