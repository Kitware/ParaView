/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQBOVWriter.h"

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
typedef vtkStreamingDemandDrivenPipeline vtkSDDPipeline;
#include "vtkMultiProcessController.h"
#include "vtkExtentTranslator.h"
#include "vtkPVXMLElement.h"

#include "BOVWriter.h"
#include "GDAMetaData.h"
#include "BOVTimeStepImage.h"
#include "Numerics.hxx"
#include "Tuple.hxx"
#include "XMLUtils.h"
#include "PrintUtils.h"
#include "SQMacros.h"
#include "postream.h"

#ifndef SQTK_WITHOUT_MPI
#include <mpi.h>
#endif

#include <algorithm>
using std::min;
using std::max;

#include <sstream>
using std::ostringstream;

// #define vtkSQBOVWriterDEBUG
// #define vtkSQBOVWriterTIME

#ifdef WIN32
  // for gethostname on windows.
  #include <Winsock2.h>
  // these are only usefull in terminals
  #undef vtkSQBOVWriterTIME
  #undef vtkSQBOVWriterDEBUG
#endif

#ifndef HOST_NAME_MAX
  #define HOST_NAME_MAX 255
#endif

#if defined vtkSQBOVWriterTIME
  #include "vtkSQLog.h"
#endif

// disbale warning about passing string literals.
#if !defined(__INTEL_COMPILER) && defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif

vtkStandardNewMacro(vtkSQBOVWriter);

//-----------------------------------------------------------------------------
vtkSQBOVWriter::vtkSQBOVWriter()
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::vtkSQBOVWriter" << endl;
  #endif

  // Initialize pipeline.
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(0);

  // Initialize variables
  this->FileName=0;
  this->FileNameChanged=false;
  this->IncrementalMetaData=0;
  this->WriteAllTimeSteps=0;
  this->TimeStepId=0;
  this->UseCollectiveIO=HINT_DISABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDirectIO=HINT_AUTOMATIC;
  this->UseDeferredOpen=HINT_DEFAULT;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->StripeSize=0;
  this->StripeCount=0;
  this->WorldRank=0;
  this->WorldSize=1;

  this->HostName[0]='\0';
  #if defined vtkSQBOVWriterDEBUG
  char hostname[HOST_NAME_MAX];
  gethostname(hostname,HOST_NAME_MAX);
  hostname[4]='\0';
  for (int i=0; i<5; ++i)
    {
    this->HostName[i]=hostname[i];
    }
  #endif

  #ifdef SQTK_WITHOUT_MPI
  vtkErrorMacro(
      << "This class requires MPI however it was built without MPI.");
  #else
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    vtkErrorMacro(
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    }
  else
    {
    MPI_Comm_size(MPI_COMM_WORLD,&this->WorldSize);
    MPI_Comm_rank(MPI_COMM_WORLD,&this->WorldRank);
    }
  #endif

  // Configure the internal writer.
  this->Writer=BOVWriter::New();

  GDAMetaData md;
  this->Writer->SetMetaData(&md);
}

//-----------------------------------------------------------------------------
vtkSQBOVWriter::~vtkSQBOVWriter()
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::~vtkSQBOVWriter" << endl;
  #endif

  this->Clear();
  this->Writer->Delete();
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::Clear()
{
  this->SetFileName(0);
  this->FileNameChanged=false;
  this->UseCollectiveIO=HINT_ENABLED;
  this->NumberOfIONodes=0;
  this->CollectBufferSize=0;
  this->UseDeferredOpen=HINT_DEFAULT;
  this->UseDataSieving=HINT_AUTOMATIC;
  this->SieveBufferSize=0;
  this->StripeSize=0;
  this->StripeCount=0;
  this->Writer->Close();
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::Initialize(vtkPVXMLElement *root)
{
  vtkPVXMLElement *elem=GetRequiredElement(root,"vtkSQBOVWriter");
  if (elem==0)
    {
    sqErrorMacro(pCerr(),"Element for vtkSQBOVWriter was not present.");
    return -1;
    }

  int cb_buffer_size=0;
  GetOptionalAttribute<int,1>(elem,"cb_buffer_size",&cb_buffer_size);
  if (cb_buffer_size)
    {
    this->SetCollectBufferSize(cb_buffer_size);
    }

  int stripe_count=0;
  GetOptionalAttribute<int,1>(elem,"stripe_count",&stripe_count);
  if (stripe_count)
    {
    this->SetStripeCount(stripe_count);
    }

  int stripe_size=0;
  GetOptionalAttribute<int,1>(elem,"stripe_size",&stripe_size);
  if (stripe_size)
    {
    this->SetStripeSize(stripe_size);
    }

  this->SetUseCollectiveIO(HINT_AUTOMATIC);
  int cb_enable=-1;
  GetOptionalAttribute<int,1>(elem,"cb_enable",&cb_enable);
  if (cb_enable==0)
    {
    this->SetUseCollectiveIO(HINT_DISABLED);
    }
  else
  if (cb_enable==1)
    {
    this->SetUseCollectiveIO(HINT_ENABLED);
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

  #if defined vtkSQBOVWriterTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  *log
    << "# ::vtkSQBOVWriter" << "\n"
    << "#   cb_buffer_size=" << cb_buffer_size << "\n"
    << "#   stripe_count=" << stripe_count << "\n"
    << "#   stripe_size=" << stripe_size << "\n"
    << "#   cb_enable=" << cb_enable << "\n"
    << "#   direct_io=" << direct_io << "\n"
    << "\n";
  #endif

  return 0;
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::SetFileName(const char* _arg)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::SetFileName" << endl;
  pCerr() << "Set FileName " << safeio(_arg) << "." << endl;
  #endif
  #if defined vtkSQBOVWriterTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQBOVWriter::SetFileName");
  #endif

  #ifndef SQTK_WITHOUT_MPI
  vtkDebugMacro(<< this->GetClassName() << ": setting FileName to " << (_arg?_arg:"(null)"));
  if (this->FileName == NULL && _arg == NULL) { return;}
  if (this->FileName && _arg && (!strcmp(this->FileName,_arg))) { return;}
  if (this->FileName) { delete [] this->FileName; }
  if (_arg)
    {
    size_t n = strlen(_arg) + 1;
    char *cp1 =  new char[n];
    const char *cp2 = (_arg);
    this->FileName = cp1;
    do { *cp1++ = *cp2++; } while (--n);
    }
  else
    {
    this->FileName = NULL;
    }

  // Close the currently opened dataset (if any).
  if (this->Writer->IsOpen())
    {
    this->Writer->Close();
    }

  // Open the newly named dataset.
  if (this->FileName)
    {
    this->Writer->SetCommunicator(MPI_COMM_WORLD);
    char mode=this->IncrementalMetaData==0?'w':'a';
    if(!this->Writer->Open(this->FileName,mode))
      {
      vtkErrorMacro("Failed to open the file \"" << safeio(this->FileName) << "\".");
      return;
      }

    #if defined vtkSQBOVWriterDEBUG
    pCerr()
      << "vtkSQBOVWriter "
      << this->HostName << " "
      << this->WorldRank
      << " Open succeeded." << endl;
    #endif
    }

  this->Modified();
  #endif

  #if defined vtkSQBOVWriterTIME
  log->EndEvent("vtkSQBOVWriter::SetFileName");
  #endif
}

//-----------------------------------------------------------------------------
bool vtkSQBOVWriter::IsOpen()
{
  return this->Writer->IsOpen();
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::WriteMetaData()
{
  this->Writer->GetMetaData()->ActivateAllArrays();
  return this->Writer->WriteMetaData();
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::GetNumberOfTimeSteps()
{
  return this->Writer->GetMetaData()->GetNumberOfTimeSteps();
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::GetTimeSteps(double *times)
{
  int n=this->Writer->GetMetaData()->GetNumberOfTimeSteps();

  for (int i=0; i<n; ++i)
    {
    times[i]=this->Writer->GetMetaData()->GetTimeStep(i);
    }
}

//-----------------------------------------------------------------------------
double vtkSQBOVWriter::GetTimeStep(int i)
{
  return this->Writer->GetMetaData()->GetTimeStep(i);
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::SetPointArrayStatus(const char *name, int status)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::SetPointArrayStatus" << endl;
  pCerr() << safeio(name) << " " << status << endl;
  #endif
  if (status)
    {
    this->Writer->GetMetaData()->ActivateArray(name);
    }
  else
    {
    this->Writer->GetMetaData()->DeactivateArray(name);
    }
  this->Modified();
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::GetPointArrayStatus(const char *name)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::GetPointArrayStatus" << endl;
  #endif
  return this->Writer->GetMetaData()->IsArrayActive(name);
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::GetNumberOfPointArrays()
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::GetNumberOfPointArrays" << endl;
  #endif
  return this->Writer->GetMetaData()->GetNumberOfArrays();
}

//-----------------------------------------------------------------------------
const char* vtkSQBOVWriter::GetPointArrayName(int idx)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::GetArrayName" << endl;
  #endif
  return this->Writer->GetMetaData()->GetArrayName(idx);
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::ClearPointArrayStatus()
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::ClearPointArrayStatus" << endl;
  #endif

  int nArrays=this->GetNumberOfPointArrays();
  for (int i=0; i<nArrays; ++i)
    {
    const char *aName=this->GetPointArrayName(i);
    this->SetPointArrayStatus(aName,0);
    }
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::SetMPIFileHints()
{
  #ifndef SQTK_WITHOUT_MPI
  int mpiOk;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
    {
    vtkErrorMacro(
      << "This class requires the MPI runtime, "
      << "you must run ParaView in client-server mode launched via mpiexec.");
    return;
    }

  MPI_Info hints;
  MPI_Info_create(&hints);

  switch (this->UseCollectiveIO)
    {
    case HINT_AUTOMATIC:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_cb_write","disable");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_cb_write","enable");
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

  if (this->CollectBufferSize>0)
    {
    ostringstream os;
    os << this->CollectBufferSize;
    MPI_Info_set(hints,"cb_buffer_size",const_cast<char *>(os.str().c_str()));
    //MPI_Info_set(hints,"striping_unit", const_cast<char *>(os.str().c_str()));
    }

  switch (this->UseDataSieving)
    {
    case HINT_AUTOMATIC:
      // do nothing, it's up to implementation.
      break;
    case HINT_DISABLED:
      MPI_Info_set(hints,"romio_ds_write","disable");
      break;
    case HINT_ENABLED:
      MPI_Info_set(hints,"romio_ds_write","enable");
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

  if (this->StripeCount>0)
    {
    ostringstream os;
    os << this->StripeCount;
    MPI_Info_set(hints,"striping_count", const_cast<char *>(os.str().c_str()));
    }

  if (this->StripeSize>0)
    {
    ostringstream os;
    os << this->StripeSize;
    MPI_Info_set(hints,"striping_unit", const_cast<char *>(os.str().c_str()));
    }

  this->Writer->SetHints(hints);

  MPI_Info_free(&hints);
  #endif
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::RequestUpdateExtent(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::RequestUpdateExtent" << endl;
  #endif

  vtkInformation *inInfo=inInfos[0]->GetInformationObject(0);

  // if we are writing all times we need to set the speciic
  // time value here.
  double time=-1.0;
  if (this->WriteAllTimeSteps)
    {
    time=this->GetTimeStep(this->TimeStepId);
    inInfo->Set(vtkSDDPipeline::UPDATE_TIME_STEP(),time);
    }

  // get the extent from the upstream source and use it
  // to compute this ranks update extent.
  CartesianExtent wholeExt;
  inInfo->Get(
        vtkSDDPipeline::WHOLE_EXTENT(),
        wholeExt.GetData());

  vtkExtentTranslator *translator=vtkExtentTranslator::New();
  translator->SetWholeExtent(wholeExt.GetData());
  translator->SetNumberOfPieces(this->WorldSize);
  translator->SetPiece(this->WorldRank);
  translator->PieceToExtent();
  CartesianExtent updateExt;
  translator->GetExtent(updateExt.GetData());
  translator->Delete();

  inInfo->Set(
        vtkSDDPipeline::UPDATE_EXTENT(),
        updateExt.GetData(),
        6);

  inInfo->Set(
        vtkSDDPipeline::UPDATE_NUMBER_OF_PIECES(),
        this->WorldSize);

  inInfo->Set(
        vtkSDDPipeline::UPDATE_PIECE_NUMBER(),
        this->WorldRank);

  #ifdef vtkSQBOVWriterDEBUG
  pCerr()
    << "WHOLE_EXTENT=" << wholeExt << " "
    << "UPDATE_EXTENT=" << updateExt << " "
    << "UPDATE_TIME_STEPS=" << time << endl;
  #endif

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQBOVWriter::RequestDataObject(
      vtkInformation* /*req*/,
      vtkInformationVector** /*inInfos*/,
      vtkInformationVector* outInfos)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::RequestDataObject" << endl;
  #endif

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::RequestInformation(
  vtkInformation*,
  vtkInformationVector**inInfos,
  vtkInformationVector* /*outInfos*/)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::RequestInformation" << endl;
  #endif

  if (!this->Writer->IsOpen())
    {
    vtkErrorMacro("No file open.");
    return 1;
    }

  vtkInformation *info=inInfos[0]->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *input
    = dynamic_cast<vtkDataSet*>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (!input)
    {
    vtkErrorMacro("No input.");
    return 1;
    }

  // configure the metadata object from pipeline info.
  BOVMetaData *md=this->Writer->GetMetaData();

  // set the dataset type
  md->SetDataSetType(input->GetClassName());

  // Determine which time steps are available.
  md->ClearTimeSteps();

  int nSteps
    = info->Length(vtkSDDPipeline::TIME_STEPS());

  vector<double> times(nSteps,0.0);

  info->Get(
        vtkSDDPipeline::TIME_STEPS(),
        &times[0]);

  for (int i=0; i<nSteps; ++i)
    {
    md->AddTimeStep(times[i]);
    }

  return 1;
}

//-----------------------------------------------------------------------------
int vtkSQBOVWriter::RequestData(
        vtkInformation *req,
        vtkInformationVector **inInfos,
        vtkInformationVector * /*outInfos*/)
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::RequestData" << endl;
  #endif
  #if defined vtkSQBOVWriterTIME
  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->StartEvent("vtkSQBOVWriter::RequestData");
  #endif

  if (!this->Writer->IsOpen())
    {
    vtkErrorMacro("No file open.");
    return 1;
    }

  BOVMetaData *md=this->Writer->GetMetaData();

  vtkInformation *info=inInfos[0]->GetInformationObject(0);

  // Get the output dataset.
  vtkDataSet *input
    = dynamic_cast<vtkDataSet *>(info->Get(vtkDataObject::DATA_OBJECT()));
  if (input==NULL)
    {
    vtkErrorMacro("Filter data has not been configured correctly.");
    return 1;
    }

  if (md->GetNumberOfTimeSteps()<1)
    {
    vtkErrorMacro("No timesteps available.");
    return 1;
    }

  // Figure out which of our time steps is closest to the requested
  // one, fallback to the 0th step.
  int stepId=md->GetTimeStep(0);
  if (info->Has(vtkSDDPipeline::UPDATE_TIME_STEP()))
    {
    double step
      = info->Get(vtkSDDPipeline::UPDATE_TIME_STEP());
    int nSteps
      = info->Length(vtkSDDPipeline::TIME_STEPS());
    double* steps =
      info->Get(vtkSDDPipeline::TIME_STEPS());

    for (int i=0; i<nSteps; ++i)
      {
      if (fequal(steps[i],step,1E-10))
        {
        stepId=md->GetTimeStep(i);
        break;
        }
      }

    #if defined vtkSQBOVWriterDEBUG
    pCerr() << "Requested time " << step << " using " << stepId << "." << endl;
    #endif
    }

  // add all of the arrays that are present to the metadata object.
  // at some point we may want to allow selection.
  md->DeactivateAllArrays();
  int nArrays=input->GetPointData()->GetNumberOfArrays();
  for(int i=0; i<nArrays; ++i)
    {
    vtkDataArray *array=input->GetPointData()->GetArray(i);
    const char *name=array->GetName();
    int nComps=array->GetNumberOfComponents();

    switch(nComps)
      {
      case 1:
        md->AddScalar(name);
        break;

      case 3:
        md->AddVector(name);
        break;

      case 9:
        // TODO -- how can I tell if it's symetric??
        md->AddTensor(name);
        break;

      default:
        vtkErrorMacro(
          << "Unsupported number of components " << nComps << " for " << name);
      }
    md->ActivateArray(name);
    }

  // A write puts data from the node centered grid in memory to
  // the cell centered grid on disk. file->nCells==memory->nPoints
  // ParaView sends the update extent to inform us of the domain decomposition.
  // The decomp is what will be writen by this process.
  // we need to translate into file index space of 0,0,0
  CartesianExtent wholeExtent;
  info->Get(
        vtkSDDPipeline::WHOLE_EXTENT(),
        wholeExtent.GetData());

  CartesianExtent updateExtent;
  info->Get(
        vtkSDDPipeline::UPDATE_EXTENT(),
        updateExtent.GetData());

  CartesianExtent domain(wholeExtent);
  CartesianExtent decomp(updateExtent);
  decomp.Shift(domain);
  domain.Shift();

  md->SetDomain(domain);
  md->SetDecomp(decomp);

  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "WHOLE_EXTENT=" << wholeExtent << endl;
  pCerr() << "domain=" << domain << endl;
  pCerr() << "UPDATE_EXTENT=" << updateExtent << endl;
  pCerr() << "decomp=" << decomp << endl;
  #endif

  md->SetDataSetType(input->GetClassName());
  if (md->DataSetTypeIsImage())
    {
    double X0[3];
    info->Get(vtkDataObject::ORIGIN(),X0);

    double dX[3];
    info->Get(vtkDataObject::SPACING(),dX);

    // translate into file index space
    for (int q=0; q<3; ++q)
      {
      int qq=2*q;

      X0[q]+=dX[q]*wholeExtent[qq];
      }

    md->SetOrigin(X0);
    md->SetSpacing(dX);

    #if defined vtkSQBOVWriterDEBUG
    pCerr() << "ORIGIN=" << Tuple<double>(X0,3) << endl;
    pCerr() << "SPACING=" << Tuple<double>(dX,3) << endl;
    #endif
    }
  else
  if (md->DataSetTypeIsRectilinear())
    {
    //vtkRectilinearGrid *rgInput=dynamic_cast<vtkRectilinearGrid*>(input);

    // TODO -- need to intitialize coordinate arrays. Gather these
    // to one rank and write them there?
    }
  else
    {
    vtkErrorMacro("Unsupported dataset type " << input->GetClassName());
    return 1;
    }

  // Construct MPI File hints for the writer.
  this->SetMPIFileHints();

  // Set the timestep to be writen.
  BOVTimeStepImage *stepImg=this->Writer->OpenTimeStep(stepId);

  // Write the selected arrays into the output.
  int ok=this->Writer->WriteTimeStep(stepImg,input,this);
  this->Writer->CloseTimeStep(stepImg);
  if (!ok)
    {
    vtkErrorMacro(
      << "Write failed." << endl << *md);
    return 1;
    }

  // when writing all time steps we can trigger re-execute here
  // the time step is set in request updated extent.
  if (this->WriteAllTimeSteps)
    {
    if (this->TimeStepId<this->GetNumberOfTimeSteps())
      {
      req->Set(vtkSDDPipeline::CONTINUE_EXECUTING(), 1);
      ++this->TimeStepId;
      }
    else
      {
      req->Remove(vtkSDDPipeline::CONTINUE_EXECUTING());
      }
    }

  #if defined vtkSQBOVWriterDEBUG
  this->Writer->PrintSelf(pCerr());
  //input->Print(pCerr());
  #endif

  #if defined vtkSQBOVWriterTIME
  log->EndEvent("vtkSQBOVWriter::RequestData");
  #endif

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQBOVWriter::Write()
{
  #if defined vtkSQBOVWriterDEBUG
  pCerr() << "=====vtkSQBOVWriter::Write" << endl;
  #endif

  if (!this->Writer->IsOpen())
    {
    vtkErrorMacro("No file open.");
    return;
    }

  this->TimeStepId=0;

  // drive the pipeline.
  this->Modified();
  this->UpdateInformation();
  this->Update();
  this->WriteMetaData();
}

//-----------------------------------------------------------------------------
void vtkSQBOVWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "FileName:        " << safeio(this->FileName) << endl;
  os << indent << "FileNameChanged: " << this->FileNameChanged << endl;
  os << indent << "Raeder: " << endl;
  this->Writer->PrintSelf(os);
  os << endl;
}
