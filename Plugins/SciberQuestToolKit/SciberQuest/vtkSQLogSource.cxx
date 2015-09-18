/*
 * Copyright 2012 SciberQuest Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of SciberQuest Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
