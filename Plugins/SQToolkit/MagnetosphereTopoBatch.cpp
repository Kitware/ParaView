/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_) 

Copyright 2008 SciberQuest Inc.
*/
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkSmartPointer.h"

#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"
#include "vtkXMLPDataSetWriter.h"

#include "SQMacros.h"
#include "postream.h"
#include "vtkSQBOVReader.h"
#include "vtkSQFieldTracer.h"
#include "vtkSQPlaneSource.h"
#include "vtkSQVolumeSource.h"
#include "vtkSQHemisphereSource.h"
#include "XMLUtils.h"

#include <sstream>
using std::ostringstream;
#include <iostream>
using std::cerr;
using std::endl;
#include <iomanip>
using std::setfill;
using std::setw;
#include <vector>
using std::vector;
#include <string>
using std::string;

#include <mpi.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

#define SQ_EXIT_ERROR 1
#define SQ_EXIT_SUCCESS 0

//*****************************************************************************
void PVTK_Exit(vtkMPIController* controller, int code)
{
  controller->Finalize();
  controller->Delete();
  vtkAlgorithm::SetDefaultExecutivePrototype(0);
  exit(code);
}

//*****************************************************************************
int IndexOf(double value, double *values, int first, int last)
{
  int mid=(first+last)/2;
  if (values[mid]==value)
    {
    return mid;
    }
  else
  if (mid!=first && values[mid]>value)
    {
    return IndexOf(value,values,first,mid-1);
    }
  else
  if (mid!=last && values[mid]<value)
    {
    return IndexOf(value,values,mid+1,last);
    }
  return -1;
}

//-----------------------------------------------------------------------------
int main(int argc, char **argv)
{
  vtkSmartPointer<vtkMPIController> controller=vtkSmartPointer<vtkMPIController>::New();

  controller->Initialize(&argc,&argv,0);
  int worldRank=controller->GetLocalProcessId();
  int worldSize=controller->GetNumberOfProcesses();

  vtkMultiProcessController::SetGlobalController(controller);

  vtkCompositeDataPipeline* cexec=vtkCompositeDataPipeline::New();
  vtkAlgorithm::SetDefaultExecutivePrototype(cexec);
  cexec->Delete();

  if (argc<4)
    {
    if (worldRank==0)
      {
      pCerr()
        << "Error: Command tail." << endl
        << " 1) /path/to/runConfig.xml" << endl
        << " 2) /path/to/file.bovm" << endl
        << " 3) /path/to/output/" << endl
        << " 4) baseFileName" << endl
        << " 5) startTime" << endl
        << " 6) endTime" << endl
        << endl;
      }
    vtkAlgorithm::SetDefaultExecutivePrototype(0);
    return SQ_EXIT_ERROR;
    }

  // distribute the configuration file name and time range
  int configNameLen=0;
  char *configName=0;

  int bovFileNameLen=0;
  char *bovFileName=0;

  int baseNameLen=0;
  char *baseName=0;

  int outputPathLen=0;
  char *outputPath=0;

  double startTime=-1.0;
  double endTime=-1.0;

  if (worldRank==0)
    {
    configNameLen=strlen(argv[1])+1;
    configName=(char *)malloc(configNameLen);
    strncpy(configName,argv[1],configNameLen);
    controller->Broadcast(&configNameLen,1,0);
    controller->Broadcast(configName,configNameLen,0);

    bovFileNameLen=strlen(argv[2])+1;
    bovFileName=(char *)malloc(bovFileNameLen);
    strncpy(bovFileName,argv[2],bovFileNameLen);
    controller->Broadcast(&bovFileNameLen,1,0);
    controller->Broadcast(bovFileName,bovFileNameLen,0);

    outputPathLen=strlen(argv[3])+1;
    outputPath=(char *)malloc(outputPathLen);
    strncpy(outputPath,argv[3],outputPathLen);
    controller->Broadcast(&outputPathLen,1,0);
    controller->Broadcast(outputPath,outputPathLen,0);

    baseNameLen=strlen(argv[4])+1;
    baseName=(char *)malloc(baseNameLen);
    strncpy(baseName,argv[4],baseNameLen);
    controller->Broadcast(&baseNameLen,1,0);
    controller->Broadcast(baseName,baseNameLen,0);

    // times are optional if not provided entire series is
    // used.
    if (argc>5)
      {
      startTime=atof(argv[5]);
      endTime=atof(argv[6]);
      }
    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }
  else
    {
    controller->Broadcast(&configNameLen,1,0);
    configName=(char *)malloc(configNameLen);
    controller->Broadcast(configName,configNameLen,0);

    controller->Broadcast(&bovFileNameLen,1,0);
    bovFileName=(char *)malloc(bovFileNameLen);
    controller->Broadcast(bovFileName,bovFileNameLen,0);


    controller->Broadcast(&outputPathLen,1,0);
    outputPath=(char *)malloc(outputPathLen);
    controller->Broadcast(outputPath,outputPathLen,0);

    controller->Broadcast(&baseNameLen,1,0);
    baseName=(char *)malloc(baseNameLen);
    controller->Broadcast(baseName,baseNameLen,0);

    controller->Broadcast(&startTime,1,0);
    controller->Broadcast(&endTime,1,0);
    }

  // read the configuration file.
  int iErr=0;
  vtkSmartPointer<vtkPVXMLParser> parser=vtkSmartPointer<vtkPVXMLParser>::New();
  parser->SetFileName(configName);
  if (parser->Parse()==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }

  // check for the semblance of a valid configuration hierarchy
  vtkPVXMLElement *root=parser->GetRootElement();
  if (root==0)
    {
    sqErrorMacro(pCerr(),"Invalid XML in file " << configName << ".");
    return SQ_EXIT_ERROR;
    }

  string requiredType("MagnetosphereTopologyBatch");
  const char *foundType=root->GetName();
  if (foundType==0 || foundType!=requiredType)
    {
    sqErrorMacro(pCerr(),
        << "This is not a valid "
        << requiredType
        << " XML hierarchy.");
    return SQ_EXIT_ERROR;
    }

  /// build the pipeline
  vtkPVXMLElement *elem;

  // reader
  elem=GetRequiredElement(root,"vtkSQBOVReader");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  iErr=0;
  const char *vectors;
  iErr+=GetRequiredAttribute(elem,"vectors",&vectors);

  int decompDims[3];
  iErr+=GetRequiredAttribute<int,3>(elem,"decomp_dims",decompDims);

  int blockCacheSize;
  iErr+=GetRequiredAttribute<int,1>(elem,"block_cache_size",&blockCacheSize);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(1);
  r->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
  r->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);
  r->SetFileName(bovFileName);
  r->SetPointArrayStatus(vectors,1);
  r->SetDecompDims(decompDims);
  r->SetBlockCacheSize(blockCacheSize);
  if (!r->IsOpen())
    {
    return SQ_EXIT_ERROR;
    }

  // earth terminator surfaces
  iErr=0;
  elem=GetRequiredElement(root,"vtkSQHemisphereSource");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  double hemiCenter[3];
  iErr+=GetRequiredAttribute<double,3>(elem,"center",hemiCenter);

  double hemiNorth[3];
  iErr+=GetRequiredAttribute<double,3>(elem,"north",hemiNorth);

  double hemiRadius;
  iErr+=GetRequiredAttribute<double,1>(elem,"radius",&hemiRadius);

  int hemiResolution;
  iErr+=GetRequiredAttribute<int,1>(elem,"resolution",&hemiResolution);

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQHemisphereSource *hs=vtkSQHemisphereSource::New();
  hs->SetCenter(hemiCenter);
  hs->SetNorth(hemiNorth);
  hs->SetRadius(hemiRadius);
  hs->SetResolution(hemiResolution);

  // seed source
  iErr=0;
  vtkAlgorithm *ss;
  const char *outFileExt;
  if ((elem=GetOptionalElement(root,"vtkSQPlaneSource"))!=NULL)
    {
    // 2D source
    outFileExt=".pvtp";

    double origin[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"origin",origin);

    double point1[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point1",point1);

    double point2[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point2",point2);

    int resolution[2];
    iErr+=GetRequiredAttribute<int,2>(elem,"resolution",resolution);

    if (iErr!=0)
      {
      sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
      return SQ_EXIT_ERROR;
      }

    vtkSQPlaneSource *ps=vtkSQPlaneSource::New();
    ps->SetOrigin(origin);
    ps->SetPoint1(point1);
    ps->SetPoint2(point2);
    ps->SetResolution(resolution);
    ps->SetImmediateMode(0);
    ss=ps;
    }
  else
  if ((elem=GetOptionalElement(root,"vtkSQVolumeSource"))!=NULL)
    {
    // 3D source
    outFileExt=".pvtu";

    double origin[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"origin",origin);

    double point1[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point1",point1);

    double point2[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point2",point2);

    double point3[3];
    iErr+=GetRequiredAttribute<double,3>(elem,"point3",point3);

    int resolution[3];
    iErr+=GetRequiredAttribute<int,3>(elem,"resolution",resolution);

    if (iErr!=0)
      {
      sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
      return SQ_EXIT_ERROR;
      }

    vtkSQVolumeSource *vs=vtkSQVolumeSource::New();
    vs->SetOrigin(origin);
    vs->SetPoint1(point1);
    vs->SetPoint2(point2);
    vs->SetPoint3(point3);
    vs->SetResolution(resolution);
    vs->SetImmediateMode(0);
    ss=vs;
    }
  else
    {
    sqErrorMacro(pCerr(),"No seed source found.");
    return SQ_EXIT_ERROR;
    }

  // field topology mapper
  iErr=0;
  elem=GetRequiredElement(root,"vtkSQFieldTracer");
  if (elem==0)
    {
    return SQ_EXIT_ERROR;
    }

  int integratorType;
  iErr+=GetRequiredAttribute<int,1>(elem,"integrator_type",&integratorType);

  double maxStepSize;
  iErr+=GetRequiredAttribute<double,1>(elem,"max_step_size",&maxStepSize);

  double maxArcLength;
  iErr+=GetRequiredAttribute<double,1>(elem,"max_arc_length",&maxArcLength);

  double minStepSize;
  int maxNumberSteps;
  double maxError;
  if (integratorType==vtkSQFieldTracer::INTEGRATOR_RK45)
    {
    GetRequiredAttribute<double,1>(elem,"min_step_size",&minStepSize);
    GetRequiredAttribute<int,1>(elem,"max_number_steps",&maxNumberSteps);
    GetRequiredAttribute<double,1>(elem,"max_error",&maxError);
    }

  double nullThreshold;
  GetRequiredAttribute<double,1>(elem,"null_threshold",&nullThreshold);

  int dynamicScheduler;
  GetRequiredAttribute<int,1>(elem,"dynamic_scheduler",&dynamicScheduler);

  int masterBlockSize;
  int workerBlockSize;
  if (dynamicScheduler)
    {
    GetRequiredAttribute<int,1>(elem,"master_block_size",&masterBlockSize);
    GetRequiredAttribute<int,1>(elem,"worker_block_size",&workerBlockSize);
    }

  if (iErr!=0)
    {
    sqErrorMacro(pCerr(),"Error: Parsing " << elem->GetName() <<  ".");
    return SQ_EXIT_ERROR;
    }

  vtkSQFieldTracer *ftm=vtkSQFieldTracer::New();
  ftm->SetMode(vtkSQFieldTracer::MODE_TOPOLOGY);
  ftm->SetIntegratorType(integratorType);
  ftm->SetMaxLineLength(maxArcLength);
  ftm->SetMaxStep(maxStepSize);
  if (integratorType==vtkSQFieldTracer::INTEGRATOR_RK45)
    {
    ftm->SetMinStep(minStepSize);
    ftm->SetMaxNumberOfSteps(maxNumberSteps);
    ftm->SetMaxError(maxError);
    }
  ftm->SetNullThreshold(nullThreshold);
  ftm->SetUseDynamicScheduler(dynamicScheduler);
  if (dynamicScheduler)
    {
    ftm->SetMasterBlockSize(masterBlockSize);
    ftm->SetWorkerBlockSize(workerBlockSize);
    }
  ftm->SetSqueezeColorMap(0);
  ftm->AddVectorInputConnection(r->GetOutputPort(0));
  ftm->AddTerminatorInputConnection(hs->GetOutputPort(0));
  ftm->AddTerminatorInputConnection(hs->GetOutputPort(1));
  ftm->AddSeedPointInputConnection(ss->GetOutputPort(0));
  ftm->SetInputArrayToProcess(
        0,
        0,
        0,
        vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vectors);

  r->Delete();
  hs->Delete();
  ss->Delete();

  vtkXMLPDataSetWriter *w=vtkXMLPDataSetWriter::New();
  //w->SetDataModeToBinary();
  w->SetDataModeToAppended();
  w->SetEncodeAppendedData(0);
  w->SetCompressorTypeToNone();
  w->AddInputConnection(0,ftm->GetOutputPort(0));
  w->SetNumberOfPieces(worldSize);
  w->SetStartPiece(worldRank);
  w->SetEndPiece(worldRank);
  w->UpdateInformation();
  //const char *ext=w->GetDefaultFileExtension();
  ftm->Delete();

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(ftm->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);

  // querry available times
  exec->UpdateInformation();
  double *times=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Get(info);
  int nTimes=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Length(info);
  if (nTimes<1)
    {
    sqErrorMacro(pCerr(),"Error: No timesteps.");
    return SQ_EXIT_ERROR;
    }


  int startTimeIdx;
  int endTimeIdx;
  if (startTime<0.0)
    {
    // if no start time was provided use entire series
    startTime=times[0];
    startTimeIdx=0;

    endTime=times[nTimes-1];
    endTimeIdx=nTimes-1;
    }
  else
    {
    // get indices of requested start and end times
    startTimeIdx=IndexOf(startTime,times,0,nTimes-1);
    if (startTimeIdx<0)
      {
      sqErrorMacro(pCerr(),"Invalid start time " << startTimeIdx << ".");
      return SQ_EXIT_ERROR;
      }

    endTimeIdx=IndexOf(endTime,times,0,nTimes-1);
    if (endTimeIdx<0)
      {
      sqErrorMacro(pCerr(),"Invalid end time " << endTimeIdx << ".");
      return SQ_EXIT_ERROR;
      }
    }

  if (worldRank==0)
    {
    pCerr()
      << "Selected " 
      << startTime << ":" << startTimeIdx
      << " to "
      << endTime << ":" << endTimeIdx
      << endl; 
    }

    ostringstream fns;
    fns
      << outputPath
      << "/"
      << baseName;

    // make a directory for this dataset
    iErr=mkdir(fns.str().c_str(),S_IRWXU|S_IXGRP);
    if (iErr<0 && (errno!=EEXIST))
      {
      char *sErr=strerror(errno);
      sqErrorMacro(pCerr(),
          << "Failed to mkdir " << fns.str() << "." << endl
          << "Error: " << sErr << ".");
      return SQ_EXIT_ERROR;
      }

  /// execute
  // run the pipeline for each time step, write the
  // result to disk.
  for (int idx=startTimeIdx,q=0; idx<=endTimeIdx; ++idx,++q)
    {
    double time=times[idx];

    exec->SetUpdateTimeStep(0,time);

    fns.str("");
    fns
      << outputPath
      << "/"
      << baseName
      << "/"
      << setfill('0') << setw(8) << time;

    // make a directory for this time step
    iErr=mkdir(fns.str().c_str(),S_IRWXU|S_IXGRP);
    if (iErr<0 && (errno!=EEXIST))
      {
      char *sErr=strerror(errno);
      sqErrorMacro(pCerr(),
          << "Failed to mkdir " << fns.str() << "."
          << "Error: " << sErr << ".");
      return SQ_EXIT_ERROR;
      }

    fns
      << "/"
      << baseName
      << outFileExt;

    w->SetFileName(fns.str().c_str());
    w->Write();

    if (worldRank==0)
      {
      pCerr() << "Wrote: " << fns.str().c_str() << "." << endl;
      }
    }
  w->Delete();

  free(configName);
  free(baseName);

  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  controller->Finalize();

  return SQ_EXIT_SUCCESS;
}
