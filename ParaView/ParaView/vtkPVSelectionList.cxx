/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSelectionList.cxx
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
#include "vtkPVSelectionList.h"
#include "vtkPVApplication.h"
#include "vtkStringList.h"
#include "vtkKWLabel.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"
#include "vtkArrayMap.txx"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVSelectionList);

int vtkPVSelectionListCommand(ClientData cd, Tcl_Interp *interp,
		     int argc, char *argv[]);


//----------------------------------------------------------------------------
vtkPVSelectionList::vtkPVSelectionList()
{
  this->CommandFunction = vtkPVSelectionListCommand;

  this->CurrentValue = 0;
  this->CurrentName = NULL;
  
  this->Label = vtkKWLabel::New();
  this->Menu = vtkKWOptionMenu::New();

  this->Names = vtkStringList::New();
}

//----------------------------------------------------------------------------
vtkPVSelectionList::~vtkPVSelectionList()
{
  this->SetCurrentName(NULL);
  
  this->Label->Delete();
  this->Label = NULL;
  this->Menu->Delete();
  this->Menu = NULL;
  this->Names->Delete();
  this->Names = NULL;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());

  this->Label->SetParent(this);
  this->Label->Create(app, "-width 18 -justify right");
  this->Script("pack %s -side left", this->Label->GetWidgetName());

  this->Menu->SetParent(this);
  this->Menu->Create(app, "");
  this->Script("pack %s -side left", this->Menu->GetWidgetName());

  char tmp[1024];
  int i, numItems = this->Names->GetLength();
  char *name;
  for(i=0; i<numItems; i++)
    {
    name = this->Names->GetString(i);
    if (name)
      {
      sprintf(tmp, "SelectCallback {%s} %d", name, i);
      this->Menu->AddEntryWithCommand(name, this, tmp);
      }
    }
  name = this->Names->GetString(this->CurrentValue);
  if (name)
    {
    this->Menu->SetValue(name);
    }

}


//----------------------------------------------------------------------------
void vtkPVSelectionList::SetLabel(const char* label) 
{
  // For getting the widget in a script.
  this->SetTraceName(label);
  this->Label->SetLabel(label);
}
  
const char *vtkPVSelectionList::GetLabel()
{
  return this->Label->GetLabel();
}
//----------------------------------------------------------------------------
void vtkPVSelectionList::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->ModifiedFlag)
    {  
    this->AddTraceEntry("$kw(%s) SetCurrentValue {%d}", this->GetTclName(), 
                         this->GetCurrentValue());
    }

  // Command to update the UI.
  pvApp->BroadcastScript("%s Set%s %d",
                         this->ObjectTclName,
                         this->VariableName,
                         this->CurrentValue); 

  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSelectionList::Reset()
{

  this->Script("%s SetCurrentValue [%s Get%s]",
               this->GetTclName(),
               this->ObjectTclName,
               this->VariableName);

  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSelectionList::AddItem(const char *name, int value)
{
  char tmp[1024];
  
  // Save for internal use
  this->Names->SetString(value, name);

  // It should be possible to add items without creating
  // the widget. This is necessary for the prototypes.
  if (this->Application)
    {
    sprintf(tmp, "SelectCallback {%s} %d", name, value);
    this->Menu->AddEntryWithCommand(name, this, tmp);
    
    if (value == this->CurrentValue)
      {
      this->Menu->SetValue(name);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SetCurrentValue(int value)
{
  char *name;

  if (this->CurrentValue == value)
    {
    return;
    }

  this->CurrentValue = value;
  name = this->Names->GetString(value);
  if (name)
    {
    this->Menu->SetValue(name);
    this->SelectCallback(name, value);
    }
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::SelectCallback(const char *name, int value)
{
  this->CurrentValue = value;
  this->SetCurrentName(name);
  
//  this->Menu->SetButtonText(name);
  this->ModifiedCallback();
}

vtkPVSelectionList* vtkPVSelectionList::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSelectionList::SafeDownCast(clone);
}

void vtkPVSelectionList::CopyProperties(vtkPVWidget* clone, 
					vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVSelectionList* pvsl = vtkPVSelectionList::SafeDownCast(clone);
  if (pvsl)
    {
    pvsl->SetLabel(this->Label->GetLabel());
    int i, numItems = this->Names->GetLength();
    char *name;
    for(i=0; i<numItems; i++)
      {
      name = this->Names->GetString(i);
      if (name)
	{
	pvsl->Names->SetString(i, name);
	}
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVSelectionList.");
    }
}

//----------------------------------------------------------------------------
int vtkPVSelectionList::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->Label->SetLabel(label);  
    }
  else
    {
    this->Label->SetLabel(this->VariableName);
    }
  
  // Extract the list of items.
  unsigned int i;
  for(i=0;i < element->GetNumberOfNestedElements(); ++i)
    {
    vtkPVXMLElement* item = element->GetNestedElement(i);
    if(strcmp(item->GetName(), "Item") != 0)
      {
      vtkErrorMacro("Found non-Item element in SelectionList.");
      return 0;
      }
    const char* itemName = item->GetAttribute("name");
    if(!itemName)
      {
      vtkErrorMacro("Item has no name.");
      return 0;
      }
    int itemValue;
    if(!item->GetScalarAttribute("value", &itemValue))
      {
      vtkErrorMacro("Item has no value.");
      return 0;
      }
    this->AddItem(itemName, itemValue);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVSelectionList::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "CurrentName: " << (this->CurrentName?this->CurrentName:"none") << endl;
  os << indent << "CurrentValue: " << this->CurrentValue << endl;
}
