/*=========================================================================

  Program:   ParaView
  Module:    vtkPVCutEntry.cxx
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
#include "vtkPVCutEntry.h"

#include "vtkPVInputMenu.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVCutEntry);
vtkCxxRevisionMacro(vtkPVCutEntry, "1.1");

vtkCxxSetObjectMacro(vtkPVCutEntry, InputMenu, vtkPVInputMenu);

//-----------------------------------------------------------------------------
int vtkPVCutEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//-----------------------------------------------------------------------------
vtkPVCutEntry::vtkPVCutEntry()
{
  this->CommandFunction = vtkPVCutEntryCommand;
  this->InputMenu = NULL;
}

//-----------------------------------------------------------------------------
vtkPVCutEntry::~vtkPVCutEntry()
{
  this->SetInputMenu(NULL);
}

//-----------------------------------------------------------------------------
int vtkPVCutEntry::GetValueRange(float range[2])
{
  if (!this->InputMenu)
    {
    return 0;
    }
  vtkPVSource* input = this->InputMenu->GetCurrentValue();
  if (!input)
    {
    return 0;
    }
  
  float bounds[6];
  input->GetDataInformation()->GetBounds(bounds);
  float length = sqrt(
    static_cast<double>(
      (bounds[1]-bounds[0])*(bounds[1]-bounds[0])+
      (bounds[3]-bounds[2])*(bounds[3]-bounds[2])+
      (bounds[5]-bounds[4])*(bounds[5]-bounds[4])));
  
  range[0] = -length;
  range[1] =  length;

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVCutEntry::CopyProperties(
  vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVCutEntry* pvce = vtkPVCutEntry::SafeDownCast(clone);
  if (pvce)
    {
    if (this->InputMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVInputMenu* im = this->InputMenu->ClonePrototype(pvSource, map);
      pvce->SetInputMenu(im);
      im->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVCutEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVCutEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                      vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the InputMenu.
  const char* input_menu = element->GetAttribute("input_menu");
  if (!input_menu)
    {
    vtkErrorMacro("No input_menu attribute.");
    return 0;
    }

  vtkPVXMLElement* ime = element->LookupElement(input_menu);
  vtkPVWidget* w = this->GetPVWidgetFromParser(ime, parser);
  vtkPVInputMenu* imw = vtkPVInputMenu::SafeDownCast(w);
  if(!imw)
    {
    if(w) { w->Delete(); }
    vtkErrorMacro("Couldn't get InputMenu widget " << input_menu);
    return 0;
    }
  imw->AddDependent(this);
  this->SetInputMenu(imw);
  imw->Delete();

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVCutEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "InputMenu: " << this->InputMenu << endl;
}
