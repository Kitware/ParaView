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
#include "vtkPlane.h"
#include "vtkCutter.h"


#include "vtkXMLPDataSetWriter.h"

#include "Tuple.hxx"
#include "SQMacros.h"
#include "postream.h"
#include "vtkSQBOVReader.h"



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

  if (argc<13)
    {
    if (worldRank==0)
      {
      pCerr()
        << "Error: Command tail." << endl
        << " 1)      time" << endl
        << " 2-4)    x_0 y_0 y_1" << endl
        << " 5-7)    x_1 y_1 z_1" << endl
        << " 8)      num_slices" << endl
        << " 9)      /path/to/file.bov" << endl
        << " 10)     /path/to/output/" << endl
        << " 11)     baseFileName" << endl
        << " 12 - N) field1 ... fieldN " << endl
        << endl;
      }
    vtkAlgorithm::SetDefaultExecutivePrototype(0);
    return SQ_EXIT_ERROR;
    }

  // distribute the configuration file name and time range
  double time;

  double X_0[3];
  double X_1[3];
  double N[3];
  double dX[3];
  int nSlices;

  int bovFileNameLen=0;
  char *bovFileName=0;

  int baseNameLen=0;
  char *baseName=0;

  int outputPathLen=0;
  char *outputPath=0;

  int nFields;
  char **fieldNames;

  if (worldRank==0)
    {
    time = atof(argv[1]);
    controller->Broadcast(&time,1,0);

    X_0[0] = atof(argv[2]);
    X_0[1] = atof(argv[3]);
    X_0[2] = atof(argv[4]);
    controller->Broadcast(X_0,3,0);

    X_1[0] = atof(argv[5]);
    X_1[1] = atof(argv[6]);
    X_1[2] = atof(argv[7]);
    controller->Broadcast(X_1,3,0);

    N[0] = X_1[0] - X_0[0];
    N[1] = X_1[1] - X_0[1];
    N[2] = X_1[2] - X_0[2];
    controller->Broadcast(N,3,0);

    nSlices = atoi(argv[8]);
    controller->Broadcast(&nSlices,1,0);

    dX[0] = N[0]/(nSlices-1);
    dX[1] = N[1]/(nSlices-1);
    dX[2] = N[2]/(nSlices-1);
    controller->Broadcast(dX,3,0);

    bovFileNameLen=strlen(argv[9])+1;
    bovFileName=(char *)malloc(bovFileNameLen);
    strncpy(bovFileName,argv[9],bovFileNameLen);
    controller->Broadcast(&bovFileNameLen,1,0);
    controller->Broadcast(bovFileName,bovFileNameLen,0);

    outputPathLen=strlen(argv[10])+1;
    outputPath=(char *)malloc(outputPathLen);
    strncpy(outputPath,argv[10],outputPathLen);
    controller->Broadcast(&outputPathLen,1,0);
    controller->Broadcast(outputPath,outputPathLen,0);

    baseNameLen=strlen(argv[11])+1;
    baseName=(char *)malloc(baseNameLen);
    strncpy(baseName,argv[11],baseNameLen);
    controller->Broadcast(&baseNameLen,1,0);
    controller->Broadcast(baseName,baseNameLen,0);

    nFields=argc-12;
    controller->Broadcast(&nFields,1,0);

    fieldNames = (char **)malloc(nFields*sizeof(char *));

    for (int i=0, argi=12; i<nFields; ++i, ++argi)
      {
      int fieldNameLen = strlen(argv[argi])+1;
      controller->Broadcast(&fieldNameLen,1,0);

      fieldNames[i] = (char *)malloc(fieldNameLen);
      fieldNames[i] = strncpy(fieldNames[i],argv[argi],fieldNameLen);
      controller->Broadcast(fieldNames[i],fieldNameLen,0);
      }

    cerr
      << "time=" << time << endl
      << "X_0=" << Tuple<double>(X_0,3) << endl
      << "X_1=" << Tuple<double>(X_1,3) << endl
      << "N=" << Tuple<double>(N,3) << endl
      << "dX=" << Tuple<double>(dX,3) << endl
      << "nSlices=" << nSlices << endl
      << "bovFileName=" << bovFileName << endl
      << "outputPath=" << outputPath << endl
      << "baseName=" << baseName << endl
      << "nFields=" << nFields << endl;
    for (int i=0; i<nFields;++i)
      {
      cerr << "feildName[" << i << "]=" << fieldNames[i] << endl;
      }
    }
  else
    {
    controller->Broadcast(&time,1,0);
    controller->Broadcast(X_0,3,0);
    controller->Broadcast(X_1,3,0);
    controller->Broadcast(N,3,0);
    controller->Broadcast(&nSlices,1,0);
    controller->Broadcast(dX,3,0);

    controller->Broadcast(&bovFileNameLen,1,0);
    bovFileName=(char *)malloc(bovFileNameLen);
    controller->Broadcast(bovFileName,bovFileNameLen,0);

    controller->Broadcast(&outputPathLen,1,0);
    outputPath=(char *)malloc(outputPathLen);
    controller->Broadcast(outputPath,outputPathLen,0);

    controller->Broadcast(&baseNameLen,1,0);
    baseName=(char *)malloc(baseNameLen);
    controller->Broadcast(baseName,baseNameLen,0);

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

  /// build the pipeline
  vtkSQBOVReader *r=vtkSQBOVReader::New();
  r->SetMetaRead(0);
  r->SetUseCollectiveIO(vtkSQBOVReader::HINT_ENABLED);
  r->SetUseDataSieving(vtkSQBOVReader::HINT_AUTOMATIC);
  r->SetFileName(bovFileName);
  if (!r->IsOpen())
    {
    sqErrorMacro(pCerr(),"Failed to open file named " << bovFileName << "."); 
    return SQ_EXIT_ERROR;
    }
  for (int i=0; i<nFields; ++i)
    {
    r->SetPointArrayStatus(fieldNames[i],1);
    }

  vtkPlane *p=vtkPlane::New();
  p->SetOrigin(X_0);
  p->SetNormal(N);

  vtkCutter *c=vtkCutter::New();
  c->SetCutFunction(p);
  p->Delete();
  c->AddInputConnection(0,r->GetOutputPort(0));
  r->Delete();

  vtkXMLPDataSetWriter *w=vtkXMLPDataSetWriter::New();
  //w->SetDataModeToBinary();
  w->SetDataModeToAppended();
  w->SetEncodeAppendedData(0);
  w->SetCompressorTypeToNone();
  w->AddInputConnection(0,c->GetOutputPort(0));
  w->SetNumberOfPieces(worldSize);
  w->SetStartPiece(worldRank);
  w->SetEndPiece(worldRank);
  w->UpdateInformation();
  //const char *ext=w->GetDefaultFileExtension();
  c->Delete();

  // initialize for domain decomposition
  vtkStreamingDemandDrivenPipeline* exec
    = dynamic_cast<vtkStreamingDemandDrivenPipeline*>(c->GetExecutive());

  vtkInformation *info=exec->GetOutputInformation(0);

  exec->SetUpdateNumberOfPieces(info,worldSize);
  exec->SetUpdatePiece(info,worldRank);

  // querry available times
  exec->UpdateInformation();
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
    sqErrorMacro(pCerr(),"Invalid time " << time << ".");
    return SQ_EXIT_ERROR;
    }

  if (worldRank==0)
    {
    cerr << "Selected " << time << ":" << timeIdx << endl; 
    }

    ostringstream fns;
    fns
      << outputPath
      << "/"
      << baseName;

    // make a directory for this dataset
    int iErr=mkdir(fns.str().c_str(),S_IRWXU|S_IXGRP);
    if (iErr<0 && (errno!=EEXIST))
      {
      char *sErr=strerror(errno);
      sqErrorMacro(pCerr(),
          << "Failed to mkdir " << fns.str() << "." << endl
          << "Error: " << sErr << ".");
      return SQ_EXIT_ERROR;
      }

  /// execute
  exec->SetUpdateTimeStep(0,time);

  // run the pipeline once for each slice
  for (int q=0; q<nSlices; ++q)
    {
    // update the plane's location
    p->SetOrigin(X_0);
    if (worldRank==0)
      {
      cerr << "Cutting at " << Tuple<double>(X_0,3) << endl;
      }

    X_0[0] += dX[0];
    X_0[1] += dX[1];
    X_0[2] += dX[2];

    // make a directory for this slice
    fns.str("");
    fns
      << outputPath
      << "/"
      << baseName
      << "/"
      << setfill('0') << setw(8) << q;
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
      << ".pvtp";

    w->SetFileName(fns.str().c_str());
    w->Write();

    if (worldRank==0)
      {
      cerr << "Wrote: " << fns.str().c_str() << "." << endl;
      }
    }
  w->Delete();

  free(bovFileName);
  free(outputPath);
  free(baseName);
  for (int i=0; i<nFields; ++i)
    {
    free(fieldNames[i]);
    }
  free(fieldNames);


  vtkAlgorithm::SetDefaultExecutivePrototype(0);

  controller->Finalize();

  return SQ_EXIT_SUCCESS;
}
