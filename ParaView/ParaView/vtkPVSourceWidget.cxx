/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourceWidget.cxx
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
#include "vtkPVSourceWidget.h"

#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"

vtkCxxRevisionMacro(vtkPVSourceWidget, "1.6.4.4");

//----------------------------------------------------------------------------
vtkPVSourceWidget::vtkPVSourceWidget()
{
  this->SourceID.ID = 0;
  this->OutputID.ID = 0;
}

//----------------------------------------------------------------------------
vtkPVSourceWidget::~vtkPVSourceWidget()
{

  if (this->OutputID.ID)
    {
    vtkPVApplication* pvApp = 
      vtkPVApplication::SafeDownCast(this->Application);
    if (pvApp)
      {
      pvApp->GetProcessModule()->DeleteStreamObject(this->OutputID);
      pvApp->GetProcessModule()->SendStreamToServer();
      }
    }

  if (this->SourceID.ID)
    {
    vtkPVApplication* pvApp = 
      vtkPVApplication::SafeDownCast(this->Application);
    if (pvApp)
      {  
      pvApp->GetProcessModule()->DeleteStreamObject(this->SourceID);
      pvApp->GetProcessModule()->SendStreamToServer();
      }
    }

  this->SourceID.ID = 0;
  this->OutputID.ID = 0;
}

//----------------------------------------------------------------------------
void vtkPVSourceWidget::SaveInBatchScript(ofstream *file)
{
  if (this->SourceID.ID)
    {
    return;
    } 
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke 
                  << this->SourceID << "GetClassName" 
                  << vtkClientServerStream::End;
  pm->SendStreamToClient();
  const char* dataClassName;
  if(pm->GetLastClientResult().GetArgument(0, 0, &dataClassName))
    {
    *file << dataClassName << " pvTemp" << this->SourceID.ID << "\n";
    }
  // This will loop through all parts and call SaveInBatchScriptForPart.
  this->Superclass::SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
void vtkPVSourceWidget::SaveInBatchScriptForPart(ofstream *file, 
                                                 vtkClientServerID sourceID)
{
  if (sourceID.ID == 0 || this->VariableName == NULL)
    {
    return;
    } 

  *file << "\t" << "pvTemp" << sourceID << " Set" << this->VariableName
        << " pvTemp" << this->SourceID.ID << endl;
}

//----------------------------------------------------------------------------
void vtkPVSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Source Tcl name: " << (this->SourceID)
     << endl;
  os << "Output Tcl Name: " << (this->OutputID)
     << endl;
}

//----------------------------------------------------------------------------
vtkClientServerID vtkPVSourceWidget::GetObjectByName(const char* name)
{
  if(!strcmp(name, "Output"))
    {
    return this->OutputID;
    }
  if(!strcmp(name, "Source"))
    {
    return this->SourceID;
    }
  vtkClientServerID id = {0};
  return id;
}

