/*=========================================================================

  Program:   ParaView
  Module:    vtkPVEnSightReaderModule.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkPVEnSightReaderModule.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVApplication.h"
#include "vtkPVFileEntry.h"
#include "vtkPVProcessModule.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVEnSightReaderModule);
vtkCxxRevisionMacro(vtkPVEnSightReaderModule, "1.45.4.2");

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
  this->FileEntry->SetVariableName("CaseFileName");
}

//----------------------------------------------------------------------------
int vtkPVEnSightReaderModule::InitializeData()
{
  int numSources = this->GetNumberOfVTKSources();
  int i;
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  for(i = 0; i < numSources; ++i)
    {
    pm->GetStream() << vtkClientServerStream::Invoke <<  this->GetVTKSourceID(i)
                    << "Update" 
                    << vtkClientServerStream::End;
    }
  pm->SendStreamToClientAndServer();
  return this->Superclass::InitializeData();
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
    for(i=0; i < numSources; ++i)
      {
      pm->GetStream() << vtkClientServerStream::Invoke << pm->GetApplicationID()
                      << "GetController"
                      << vtkClientServerStream::End;
      pm->GetStream() << vtkClientServerStream::Invoke << this->GetVTKSourceID(i) 
                      << "SetController"
                      << vtkClientServerStream::LastResult
                      << vtkClientServerStream::End;
      pm->SendStreamToServer();
      }
    }
  return this->Superclass::ReadFileInformation(fname);
}
