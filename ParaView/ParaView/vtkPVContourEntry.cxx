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
#include "vtkPVContourWidgetProperty.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVContourEntry);
vtkCxxRevisionMacro(vtkPVContourEntry, "1.28.2.8");

vtkCxxSetObjectMacro(vtkPVContourEntry, ArrayMenu, vtkPVArrayMenu);

//-----------------------------------------------------------------------------
int vtkPVContourEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);


//-----------------------------------------------------------------------------
vtkPVContourEntry::vtkPVContourEntry()
{
  this->CommandFunction = vtkPVContourEntryCommand;

  this->SuppressReset = 1;
  
  this->AcceptCalled = 0;
  this->Property = NULL;
  
  this->ArrayMenu = NULL;
}

//-----------------------------------------------------------------------------
vtkPVContourEntry::~vtkPVContourEntry()
{
  this->SetPVSource(NULL);
  this->SetProperty(NULL);
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
  int i;
  int numContours;

  if (sourceTclName == NULL)
    {
    return;
    }

  this->Superclass::AcceptInternal(sourceTclName);

  numContours = this->ContourValues->GetNumberOfContours();

  char **cmds = new char*[numContours+1];
  int *numScalars = new int[numContours+1];

  this->UpdateProperty();
  
  cmds[0] = new char[20];
  sprintf(cmds[0], "SetNumberOfContours");
  numScalars[0] = 1;
  
  for (i = 0; i < numContours; i++)
    {
    cmds[i+1] = new char[9];
    sprintf(cmds[i+1], "SetValue");
    numScalars[i+1] = 2;
    }
  
  this->Property->SetVTKSourceTclName(sourceTclName);
  this->Property->SetVTKCommands(numContours+1, cmds, numScalars);
  this->Property->AcceptInternal();
  
  for (i = 0; i < numContours+1; i++)
    {
    delete [] cmds[i];
    }
  delete [] cmds;
  delete [] numScalars;
  
  this->AcceptCalled = 1;
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
void vtkPVContourEntry::ResetInternal()
{
  int i;
  int numContours;
  
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  numContours = (this->Property->GetNumberOfScalars()-1)/2;
  float *scalars = this->Property->GetScalars();
  
  // The widget has been modified.  
  // Now set the widget back to reflect the contours in the filter.
  this->ContourValues->SetNumberOfContours(0);
  for (i = 0; i < numContours; i++)
    {
    this->AddValueInternal(scalars[2*(i+1)]);
    }
  this->Update();

  // Since the widget now matches the fitler, it is no longer modified.
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }
  
  ai->SetLabelAndScript(this->GetTraceName(), NULL);
  ai->SetCurrentProperty(this->Property);
  if (this->UseWidgetRange)
    {
    ai->SetTimeStart(this->WidgetRange[0]);
    ai->SetTimeEnd(this->WidgetRange[1]);
    }
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
void vtkPVContourEntry::UpdateProperty()
{
  int numContours = this->ContourValues->GetNumberOfContours();
  float *scalars = new float[2*numContours+1];
  scalars[0] = numContours;
  int i;
  
  for (i = 0; i < numContours; i++)
    {
    scalars[2*i+1] = i;
    scalars[2*(i+1)] = this->ContourValues->GetValue(i);
    }
  this->Property->SetScalars(2*numContours+1, scalars);
  delete [] scalars;
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVContourWidgetProperty::SafeDownCast(prop);
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVContourEntry::CreateAppropriateProperty()
{
  return vtkPVContourWidgetProperty::New();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ArrayMenu: " << this->GetArrayMenu() << endl;
}
