/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVObjectWidget.cxx
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
#include "vtkPVObjectWidget.h"

#include "vtkArrayMap.txx"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"
#include "vtkPVProcessModule.h"
#include <vtkstd/string>

vtkCxxRevisionMacro(vtkPVObjectWidget, "1.13");

//----------------------------------------------------------------------------
vtkPVObjectWidget::vtkPVObjectWidget()
{
  this->ObjectID.ID = 0;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkPVObjectWidget::~vtkPVObjectWidget()
{
  this->SetVariableName(NULL);
}


//----------------------------------------------------------------------------
void vtkPVObjectWidget::SaveInBatchScriptForPart(ofstream *file,
                                                 vtkClientServerID sourceID)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule(); 
 
  if (sourceID.ID == 0 || this->VariableName == NULL)
    {
    vtkErrorMacro(<< this->GetClassName()
                  << " must not have SaveInBatchScript method.");
    return;
    } 

  *file << "\t" << "pvTemp" << sourceID << " Set" << this->VariableName;
  ostrstream str;
  str << "Get" << this->VariableName << ends;
  pm->GetStream() << vtkClientServerStream::Invoke << this->ObjectID
                  << str.str() 
                  << vtkClientServerStream::End;
  pm->SendStreamToClient();
  ostrstream result;
  pm->GetLastClientResult().PrintArgumentValue(result, 0,0);
  result << ends;
  *file << " " << result.str() << "\n";
  delete [] result.str();
  delete [] str.str();
}

//----------------------------------------------------------------------------
vtkPVObjectWidget* vtkPVObjectWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVObjectWidget::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
void vtkPVObjectWidget::CopyProperties(vtkPVWidget* clone, 
                                       vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVObjectWidget* pvow = vtkPVObjectWidget::SafeDownCast(clone);
  if (pvow)
    {
    pvow->SetVariableName(this->VariableName);
    pvow->SetObjectID(pvSource->GetVTKSourceID());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVObjectWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVObjectWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  const char* variable = element->GetAttribute("variable");
  if(variable)
    {
    this->SetVariableName(variable);
    }
  return 1;
}


//----------------------------------------------------------------------------
void vtkPVObjectWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "ObjectID: " << (this->ObjectID.ID)
     << endl;
  os << "VariableName: " << (this->VariableName?this->VariableName:"none") 
     << endl;
}
