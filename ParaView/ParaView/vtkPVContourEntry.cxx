/*=========================================================================

  Program:   ParaView
  Module:    vtkPVContourEntry.cxx
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
#include "vtkPVContourEntry.h"

#include "vtkContourValues.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVArrayInformation.h"
#include "vtkPVArrayMenu.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVContourEntry);
vtkCxxRevisionMacro(vtkPVContourEntry, "1.33");

vtkCxxSetObjectMacro(vtkPVContourEntry, ArrayMenu, vtkPVArrayMenu);

//-----------------------------------------------------------------------------
int vtkPVContourEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//-----------------------------------------------------------------------------
vtkPVContourEntry::vtkPVContourEntry()
{
  this->CommandFunction = vtkPVContourEntryCommand;

  this->ArrayMenu = NULL;
}

//-----------------------------------------------------------------------------
vtkPVContourEntry::~vtkPVContourEntry()
{
  this->SetArrayMenu(NULL);
}

//-----------------------------------------------------------------------------
int vtkPVContourEntry::GetValueRange(float range[2])
{
  if (!this->ArrayMenu)
    {
    vtkErrorMacro("Array menu has not been set.");
    return 0;
    }

  vtkPVArrayInformation* ai = this->ArrayMenu->GetArrayInformation();
  if (ai == NULL || ai->GetName() == NULL)
    {
    return 0;
    }

  ai->GetComponentRange(0, range);

  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AcceptInternal(const char* sourceTclName)
{
  if (sourceTclName == NULL)
    {
    return;
    }

  this->Superclass::AcceptInternal(sourceTclName);

  int numContours = this->ContourValues->GetNumberOfContours();

  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->BroadcastScript("%s SetNumberOfContours %d",
                         sourceTclName, 
                         numContours);
  
  for (int i = 0; i < numContours; i++)
    {
    float value = this->ContourValues->GetValue(i);
    pvApp->BroadcastScript("%s SetValue %d %f",
                           sourceTclName, 
                           i, 
                           value);
    }
}


//-----------------------------------------------------------------------------
void vtkPVContourEntry::SaveInBatchScriptForPart(ofstream *file,
                                                 const char* sourceTclName)
{
  int i;
  float value;
  int numContours;

  numContours = this->ContourValues->GetNumberOfContours();

  for (i = 0; i < numContours; i++)
    {
    value = this->ContourValues->GetValue(i);
    *file << "\t";
    *file << sourceTclName << " SetValue " 
          << i << " " << value << endl;
    }
}

//-----------------------------------------------------------------------------
// If we had access to the ContourValues object of the filter,
// this would be much easier.  We would not have to rely on Tcl calls.
void vtkPVContourEntry::ResetInternal(const char* sourceTclName)
{
  int i;
  int numContours;
  
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }
  this->Script("%s GetNumberOfContours", 
               sourceTclName);
  numContours = this->GetIntegerResult(this->Application);

  // The widget has been modified.  
  // Now set the widget back to reflect the contours in the filter.
  this->ContourValues->SetNumberOfContours(0);
  for (i = 0; i < numContours; i++)
    {
    this->Script("%s AddValueInternal [%s GetValue %d]", 
                 this->GetTclName(),
                 sourceTclName, i);
    }
  this->Update();

  // Since the widget now matches the fitler, it is no longer modified.
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  char script[500];
  
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }
  
  sprintf(script, "%s SetValue 0 $pvTime", 
          this->PVSource->GetVTKSourceTclName());

  ai->SetLabelAndScript(this->GetTraceName(), script);
  sprintf(script, "AnimationMenuCallback $kw(%s)", ai->GetTclName());
  ai->SetSaveStateScript(script);
  ai->SetSaveStateObject(this);
  ai->Update();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::CopyProperties(
  vtkPVWidget* clone, 
  vtkPVSource* pvSource,
  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVContourEntry* pvce = vtkPVContourEntry::SafeDownCast(clone);
  if (pvce)
    {
    if (this->ArrayMenu)
      {
      // This will either clone or return a previously cloned
      // object.
      vtkPVArrayMenu* am = this->ArrayMenu->ClonePrototype(pvSource, map);
      pvce->SetArrayMenu(am);
      am->Delete();
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVContourEntry.");
    }
}

//-----------------------------------------------------------------------------
int vtkPVContourEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                         vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the ArrayMenu.
  const char* array_menu = element->GetAttribute("array_menu");
  if(array_menu)
    {
    vtkPVXMLElement* ame = element->LookupElement(array_menu);
    if (!ame)
      {
      vtkErrorMacro("Couldn't find ArrayMenu element " << array_menu);
      return 0;
      }
    vtkPVWidget* w = this->GetPVWidgetFromParser(ame, parser);
    vtkPVArrayMenu* amw = vtkPVArrayMenu::SafeDownCast(w);
    if(!amw)
      {
      if(w) { w->Delete(); }
      vtkErrorMacro("Couldn't get ArrayMenu widget " << array_menu);
      return 0;
      }
    amw->AddDependent(this);
    this->SetArrayMenu(amw);
    amw->Delete();  
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayMenu: " << this->GetArrayMenu() << endl;
}
