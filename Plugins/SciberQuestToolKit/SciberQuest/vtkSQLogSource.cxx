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
#include "postream.h"

// #define SQTK_DEBUG

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkSQLogSource);

//----------------------------------------------------------------------------
vtkSQLogSource::vtkSQLogSource()
          :
      GlobalLevel(0),
      FileName(NULL)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::vtkSQLogSource" << std::endl;
  #endif

  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
}

//----------------------------------------------------------------------------
vtkSQLogSource::~vtkSQLogSource()
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::~vtkSQLogSource" << std::endl;
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
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::Initialize" << std::endl;
  #endif

  return vtkSQLog::GetGlobalInstance()->Initialize(root);
}

//-----------------------------------------------------------------------------
void vtkSQLogSource::SetGlobalLevel(int level)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::SetGlobalLevel" << std::endl;
  #endif

  if (this->GlobalLevel==level) return;

  this->GlobalLevel=level;

  vtkSQLog *log=vtkSQLog::GetGlobalInstance();
  log->SetGlobalLevel(level);
}

//----------------------------------------------------------------------------
int vtkSQLogSource::RequestInformation(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::RequestInformation" << std::endl;
  #endif

  (void)req;
  (void)inInfos;
  (void)outInfos;

  return 1;
}

//----------------------------------------------------------------------------
int vtkSQLogSource::RequestData(
      vtkInformation *req,
      vtkInformationVector **inInfos,
      vtkInformationVector *outInfos)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::RequestData" << std::endl;
  //this->Print(std::cerr);
  #endif

  (void)req;
  (void)inInfos;
  (void)outInfos;

  return 1;
}

//----------------------------------------------------------------------------
void vtkSQLogSource::PrintSelf(ostream& os, vtkIndent indent)
{
  #ifdef SQTK_DEBUG
  pCerr() << "=====vtkSQLogSource::PrintSelf" << std::endl;
  #endif

  (void)indent;

  const char *fileName = (this->FileName==NULL?"NULL":this->FileName);
  os
    << "GlobalLevel=" << this->GlobalLevel << std::endl
    << "FileName=" << fileName << std::endl
    << std::endl;

  // this->Superclass::PrintSelf(os,indent);
}
