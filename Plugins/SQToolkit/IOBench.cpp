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
#include "vtkDataSetAlgorithm.h"
#include "vtkDataSet.h"
#include "vtkVPICReader.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleVectorKey.h"
#include "vtkSmartPointer.h"

#include "vtkPlane.h"
#include "vtkCutter.h"
#include "vtkXMLPDataSetWriter.h"

#include "Tuple.hxx"
#include "SQMacros.h"
#include "postream.h"
#include "vtkSQBOVReader.h"
#include "vtkSQLog.h"


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

  if (argc<6)
    {
    if (worldRank==0)
      {
      pCerr()
        << "Error: Command tail." << endl
        << " 1)      reader (bov, bovc, vpic)" << endl
        << " 2)      /path/to/dataset/file.ext" << endl
        << " 3)      time" << endl
        << " 4)      /path/to/logfile/log.txt" << endl
        << " 5 - N)  fieldName1 ... fieldNameN" << endl
        << endl;
      }
    vtkAlgorithm::SetDefaultExecutivePrototype(0);
    return SQ_EXIT_ERROR;
    }

  vtkSQLog *log=vtkSQLog::New();
  log->StartEvent("TotalRunTime");

  // distribute the configuration file name and time range
  int readerIdLen;
  char *readerId;

  int inputFileNameLen=0;
  char *inputFileName=0;

  double time;

  int logFileNameLen=0;
  char *logFileName=0;

  int nFields;
  char **fieldNames;

  if (worldRank==0)
    {
    cerr << "starting" << endl;
    readerIdLen=strlen(argv[1])+1;
    readerId=(char *)malloc(readerIdLen);
    strncpy(readerId,argv[1],readerIdLen);
    controller->Broadcast(&readerIdLen,1,0);
    controller->Broadcast(readerId,readerIdLen,0);
    cerr << readerId << endl;

    inputFileNameLen=strlen(argv[2])+1;
    inputFileName=(char *)malloc(inputFileNameLen);
    strncpy(inputFileName,argv[2],inputFileNameLen);
    controller->Broadcast(&inputFileNameLen,1,0);
    controller->Broadcast(inputFileName,inputFileNameLen,0);
    cerr << inputFileName << endl;

    time = atof(argv[3]);
    controller->Broadcast(&time,1,0);
    cerr << time << endl;

    logFileNameLen=strlen(argv[4])+1;
    logFileName=(char *)malloc(logFileNameLen);
    strncpy(logFileName,argv[4],logFileNameLen);
    controller->Broadcast(&logFileNameLen,1,0);
    controller->Broadcast(logFileName,logFileNameLen,0);
    cerr << logFileName << endl;

    nFields=argc-5;
    cerr << nFields << endl;
    controller->Broadcast(&nFields,1,0);

    fieldNames = (char **)malloc(nFields*sizeof(char *));

    for (int i=0, argi=5; i<nFields; ++i, ++argi)
      {
      int fieldNameLen = strlen(argv[argi])+1;
      controller->Broadcast(&fieldNameLen,1,0);

      fieldNames[i] = (char *)malloc(fieldNameLen);
      fieldNames[i] = strncpy(fieldNames[i],argv[argi],fieldNameLen);
      controller->Broadcast(fieldNames[i],fieldNameLen,0);
      }

    cerr
      << "readerId=" << readerId << endl
      << "inputFileName=" << inputFileName << endl
      << "time=" << time << endl
      << "logFileName=" << logFileName << endl
      << "nFields=" << nFields << endl;
    for (int i=0; i<nFields;++i)
      {
      cerr << "feildName[" << i << "]=" << fieldNames[i] << endl;
      }

    }
  else
    {
    controller->Broadcast(&readerIdLen,1,0);
    readerId=(char *)malloc(readerIdLen);
    controller->Broadcast(readerId,readerIdLen,0);

    controller->Broadcast(&inputFileNameLen,1,0);
    inputFileName=(char *)malloc(inputFileNameLen);
    controller->Broadcast(inputFileName,inputFileNameLen,0);

    controller->Broadcast(&time,1,0);

    controller->Broadcast(&logFileNameLen,1,0);
    logFileName=(char *)malloc(logFileNameLen);
    controller->Broadcast(logFileName,logFileNameLen,0);

    controller->Broadcast(&nFields,1,0);

    fieldNames = (char **)malloc(nFields*sizeof(char *));
    for (int i=0; i<nFields; ++i)
      {
      int fieldNameLen;
      controller->Broadcast(&fieldNameLen,1,0);

      fieldNames[i] = (char *)malloc(fieldNameLen);
      controller->Broadcast(fieldNames[i],fieldNameLen,0);
      }
    }

  string readerIdStr=readerId;
  if ((readerIdStr!="bov")&&(readerIdStr!="vpic"))
    {
    sqErrorMacro(pCerr(),"Invalid readerId " << readerId << ".");
    return SQ_EXIT_ERROR;
    }

  /// build the pipeline
  vtkPlane *p=vtkPlane::New();
  p->SetOrigin(0,1,0);
  p->SetNormal(0,1,0);

  vtkCutter *c=vtkCutter::New();
  c->SetCutFunction(p);
  p->Delete();



  vtkSQBOVReader *bovr=0;
  vtkVPICReader *vpicr=0;
  vtkDataSetAlgorithm *a=0;
  if (readerIdStr=="bov")
    {
    bovr=vtkSQBOVReader::New();
    a=dynamic_cast<vtkDataSetAlgorithm*>(bovr);

    bovr->SetMetaRead(0);
    bovr->SetUseCollectiveIO(vtkSQBOVReader::HINT_DISABLED);
    bovr->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);

    log->StartEvent("SetFileName");
    bovr->SetFileName(inputFileName);
    log->EndEvent("SetFileName");

    if (!bovr->IsOpen())
      {
      sqErrorMacro(pCerr(),
          << "Failed to open file named "
          << inputFileName << ".");
      return SQ_EXIT_ERROR;
      }

    c->AddInputConnection(0,bovr->GetOutputPort(0));
    bovr->Delete();
    }
  else
  if (readerIdStr=="vpic")
    {
    vpicr=vtkVPICReader::New();
    a=dynamic_cast<vtkDataSetAlgorithm*>(vpicr);

    log->StartEvent("SetFileName");
    vpicr->SetFileName(inputFileName);
    log->EndEvent("SetFileName");

    c->AddInputConnection(0,vpicr->GetOutputPort(0));
    vpicr->Delete();
    }

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(c->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);

  // querry available times
  log->StartEvent("UpdateInfo");
  exec->UpdateInformation();
  log->EndEvent("UpdateInfo");

  double *times=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Get(info);
  int nTimes=vtkStreamingDemandDrivenPipeline::TIME_STEPS()->Length(info);
  if (nTimes<1)
    {
    sqErrorMacro(pCerr(),"No timesteps.");
    return SQ_EXIT_ERROR;
    }

  int timeIdx;
  // get indices of requested times
  timeIdx=IndexOf(time,times,0,nTimes-1);
  if (timeIdx<0)
    {
    sqErrorMacro(pCerr(),
        << "Invalid time " << time << "." << endl
        << "Available times are " << times << "."
        );
    return SQ_EXIT_ERROR;
    }

  if (worldRank==0)
    {
    cerr << "Selected " << time << ":" << timeIdx << endl;
    }

  /// execute
  exec->SetUpdateTimeStep(0,time);

  // run the pipeline
  if (readerIdStr=="bov")
    {
    for (int i=0; i<nFields; ++i)
      {
      bovr->SetPointArrayStatus(fieldNames[i],1);
      }
    }
  else
  if (readerIdStr=="vpic")
    {
    vpicr->DisableAllPointArrays();
    for (int i=0; i<nFields; ++i)
      {
      vpicr->SetPointArrayStatus(fieldNames[i],1);
      }
    }

  log->StartEvent("RequestData");
  vtkDataSet *d=c->GetOutput();
  d->Update();
  log->EndEvent("RequestData");

  c->Delete();


  free(readerId);
  free(inputFileName);
  free(logFileName);
  for (int i=0; i<nFields; ++i)
    {
    free(fieldNames[i]);
    }
  free(fieldNames);

  log->EndEvent("TotalRunTime");
  log->WriteLog(0,logFileName);
  log->Delete();

  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  controller->Finalize();

  return SQ_EXIT_SUCCESS;
}

