/*
   ____    _ __           ____               __    ____
  / __/___(_) /  ___ ____/ __ \__ _____ ___ / /_  /  _/__  ____
 _\ \/ __/ / _ \/ -_) __/ /_/ / // / -_|_-</ __/ _/ // _ \/ __/
/___/\__/_/_.__/\__/_/  \___\_\_,_/\__/___/\__/ /___/_//_/\__(_)

Copyright 2012 SciberQuest Inc.
*/
#include "vtkSQLogSource.h"

#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPolyData.h"
#include "vtkSQLog.h"
#include "Tuple.hxx"
#include "SQMacros.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQLogSource);

//----------------------------------------------------------------------------
vtkSQLogSource::vtkSQLogSource()
          :
      GlobalLevel(0),
      FileName(NULL)
{
  #ifdef vtkSQLogSourceDEBUG
  cerr << "=====vtkSQLogSource::vtkSQLogSource" << endl;
  #endif

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQLogSource::~vtkSQLogSource()
{
  #ifdef vtkSQLogSourceDEBUG
  cerr << "=====vtkSQLogSource::~vtkSQLogSource" << endl;
  #endif

  if (this->GlobalLevel && this->FileName)
    {
    vtkSQLog *log=vtkSQLog::GetGlobalInstance();

    log->SetFileName(this->FileName);
    log->Update();
    log->Write();

    this->SetFileName(NULL);
    this->SetGlobalLevel(0);
    }
}

//-----------------------------------------------------------------------------
int vtkSQLogSource::Initialize(vtkPVXMLElement *root)
{
  #ifdef vtkSQLogSourceDEBUG
  pCerr() << "=====vtkSQLogSource::Initialize" << endl;
  #endif

  return vtkSQLog::GetGlobalInstance()->Initialize(root);
}

//-----------------------------------------------------------------------------
void vtkSQLogSource::SetGlobalLevel(int level)
{
  #ifdef vtkSQLogSourceDEBUG
  pCerr() << "=====vtkSQLogSource::Initialize" << endl;
  #endif

  if (this->GlobalLevel==level) return;

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->SetGlobalLevel(level);
}

//----------------------------------------------------------------------------
int vtkSQLogSource::RequestInformation(
      vtkInformation* /*req*/,
      vtkInformationVector** inInfos,
      vtkInformationVector* outInfos)
{
  #ifdef vtkSQLogSourceDEBUG
  cerr << "=====vtkSQLogSource::RequestInformation" << endl;
  #endif

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQLogSource::RequestData(
      vtkInformation * /*req*/,
      vtkInformationVector ** /*inInfos*/,
      vtkInformationVector *outInfos)
{
  #ifdef vtkSQLogSourceDEBUG
  cerr << "=====vtkSQLogSource::RequestData" << endl;
  this->Print(cerr);
  #endif

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQLogSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef vtkSQLogSourceDEBUG
  cerr << "=====vtkSQLogSource::PrintSelf" << endl;
  #endif
  // this->Superclass::PrintSelf(os,indent);
}
