/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightReaderModule.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVEnSightReaderModule.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"
#include "vtkPVColorMap.h"
#include "vtkSMPartDisplay.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightReaderModule);
vtkCxxRevisionMacro(vtkPVEnSightReaderModule, "1.56.2.1");

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::vtkPVEnSightReaderModule()
{
  this->AddFileEntry = 1;
  this->PackFileEntry = 0;
  this->UpdateSourceInBatch = 1;
}

//----------------------------------------------------------------------------
vtkPVEnSightReaderModule::~vtkPVEnSightReaderModule()
{
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::CreateProperties()
{
  this->Superclass::CreateProperties();
  this->FileEntry->SetSMPropertyName("CaseFileName");
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::InitializeData()
{
  int numSources = this->GetNumberOfVTKSources();
  int i;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkClientServerStream stream;
  for(i = 0; i < numSources; ++i)
    {
    stream << vtkClientServerStream::Invoke 
           <<  this->GetVTKSourceID(i) << "Update" 
           << vtkClientServerStream::End;
    }
  pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
  return this->Superclass::InitializeData();
}

//----------------------------------------------------------------------------
void vtkPVEnSightReaderModule::SaveInBatchScript(ofstream *file)
{
  if (this->VisitedFlag)
    {
    return;
    }

  this->SaveFilterInBatchScript(file);
  *file << "  $pvTemp" <<  this->GetVTKSourceID(0)
        << " UpdatePipeline" 
        << endl;
  // Add the mapper, actor, scalar bar actor ...
  if (this->GetVisibility())
    {
    if (this->PVColorMap)
      {
      this->PVColorMap->SaveInBatchScript(file);
      }
#if !defined(PARAVIEW_USE_SERVERMANAGER_RENDERING)
    vtkSMPartDisplay *partD = this->GetPartDisplay();
    if (partD)
      {
      partD->SaveInBatchScript(file, this->GetProxy());
      }
#endif
    }
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::ReadFileInformation(const char* fname)
{
  // If this is a vtkPVEnSightMasterServerReader, set the controller.
  if(strcmp(this->SourceClassName, "vtkPVEnSightMasterServerReader") == 0)
    {
    int i;
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    int numSources = this->GetNumberOfVTKSources();
    vtkClientServerStream stream;
    for(i=0; i < numSources; ++i)
      {
      stream << vtkClientServerStream::Invoke 
             << pm->GetProcessModuleID() << "GetController"
             << vtkClientServerStream::End;
      stream << vtkClientServerStream::Invoke 
             << this->GetVTKSourceID(i) << "SetController" << vtkClientServerStream::LastResult
             << vtkClientServerStream::End;
      }
    pm->SendStream(vtkProcessModule::DATA_SERVER, stream);
    }
  return this->Superclass::ReadFileInformation(fname);
}
