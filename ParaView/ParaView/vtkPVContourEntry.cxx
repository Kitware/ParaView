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

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"

int vtkPVContourEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContourEntry::vtkPVContourEntry()
{
  this->CommandFunction = vtkPVContourEntryCommand;
  
  this->ContourValuesLabel = vtkKWLabel::New();
  this->ContourValuesList = vtkKWListBox::New();
  this->NewValueFrame = vtkKWWidget::New();
  this->NewValueLabel = vtkKWLabel::New();
  this->NewValueEntry = vtkKWEntry::New();
  this->AddValueButton = vtkKWPushButton::New();
  this->DeleteValueButton = vtkKWPushButton::New();
}

//----------------------------------------------------------------------------
vtkPVContourEntry::~vtkPVContourEntry()
{
  this->ContourValuesLabel->Delete();
  this->ContourValuesLabel = NULL;
  this->ContourValuesList->Delete();
  this->ContourValuesList = NULL;
  this->NewValueLabel->Delete();
  this->NewValueLabel = NULL;
  this->NewValueEntry->Delete();
  this->NewValueEntry = NULL;
  this->AddValueButton->Delete();
  this->AddValueButton = NULL;
  this->NewValueFrame->Delete();
  this->NewValueFrame = NULL;
  this->DeleteValueButton->Delete();
  this->DeleteValueButton = NULL;

  this->SetPVSource(NULL);
}

//----------------------------------------------------------------------------
vtkPVContourEntry* vtkPVContourEntry::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVContourEntry");
  if(ret)
    {
    return (vtkPVContourEntry*)ret;
    }
  return new vtkPVContourEntry;
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::SetLabel(const char* str)
{
  this->ContourValuesLabel->SetLabel(str);
  this->SetTraceName(str);
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::Create(vtkKWApplication *app)
{
  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());
  
  this->ContourValuesLabel->SetParent(this);
  this->ContourValuesLabel->Create(app, "");
  
  this->ContourValuesList->SetParent(this);
  this->ContourValuesList->Create(app, "");
  this->ContourValuesList->SetHeight(5);
  this->ContourValuesList->SetBalloonHelpString("List of the current contour values");
  this->Script("bind %s <Delete> {%s DeleteValueCallback}",
               this->ContourValuesList->GetWidgetName(),
               this->GetTclName());
  // We need focus for delete binding.
  this->Script("bind %s <Enter> {focus %s}",
               this->ContourValuesList->GetWidgetName(),
               this->ContourValuesList->GetWidgetName());
  
  this->NewValueFrame->SetParent(this);
  this->NewValueFrame->Create(app, "frame", "");
  
  
  this->Script("pack %s %s %s",
               this->ContourValuesLabel->GetWidgetName(),
               this->ContourValuesList->GetWidgetName(),
               this->NewValueFrame->GetWidgetName());
  
  this->NewValueLabel->SetParent(this->NewValueFrame);
  this->NewValueLabel->Create(app, "");
  this->NewValueLabel->SetLabel("New Value:");
  this->NewValueLabel->SetBalloonHelpString("Enter a new contour value");
  
  this->NewValueEntry->SetParent(this->NewValueFrame);
  this->NewValueEntry->Create(app, "");
  this->NewValueEntry->SetValue("");
  this->NewValueEntry->SetBalloonHelpString("Enter a new contour value");
  this->Script("bind %s <KeyPress-Return> {%s AddValueCallback}",
               this->NewValueEntry->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->NewValueEntry->GetWidgetName(), this->GetTclName());
  
  this->AddValueButton->SetParent(this->NewValueFrame);
  this->AddValueButton->Create(app, "-text {Add}");
  this->AddValueButton->SetCommand(this, "AddValueCallback");
  this->AddValueButton->SetBalloonHelpString("Add the new contour value to the contour values list");
  
  this->Script("pack %s %s %s -side left",
               this->NewValueLabel->GetWidgetName(),
               this->NewValueEntry->GetWidgetName(),
               this->AddValueButton->GetWidgetName());
  
  this->DeleteValueButton->SetParent(this->NewValueFrame);
  this->DeleteValueButton->Create(app, "-text {Delete}");
  this->DeleteValueButton->SetCommand(this, "DeleteValueCallback");
  this->DeleteValueButton->SetBalloonHelpString("Remove the currently selected contour value from the list");

  this->Script("pack %s -side left",
               this->DeleteValueButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::AddValueCallback()
{
  if (strcmp(this->NewValueEntry->GetValue(), "") == 0)
    {
    return;
    }

  this->ContourValuesList->AppendUnique(this->NewValueEntry->GetValue());
  this->NewValueEntry->SetValue("");
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::DeleteValueCallback()
{
  int index;
  int num, idx;
  
  // First look for slected values in the value list.
  index = this->ContourValuesList->GetSelectionIndex();
  if (index == -1)
    {
    num = this->ContourValuesList->GetNumberOfItems();
    // Next look for values in the entry box.
    if (strcmp(this->NewValueEntry->GetValue(), "") != 0)
      {
      // Find the index of the value in the entry box.
      // If the entry value is not in the list,
      // this will just clear the entry and return.
      for (idx = 0; idx < num && index < 0; ++idx)
        {
        if (strcmp(this->NewValueEntry->GetValue(),
                   this->ContourValuesList->GetItem(idx)) == 0)
          {
          index = idx;
          }
        }
      }
    else
      {
      // Finally just delete the last in the list.
      index = num - 1;
      }
    }

  if ( index >= 0 )
    {
    this->ContourValuesList->DeleteRange(index, index);
    }
  this->ModifiedCallback();
  this->NewValueEntry->SetValue("");
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::AddValue(char *val)
{
  this->ContourValuesList->AppendUnique(val);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::RemoveAllValues()
{
  this->ContourValuesList->DeleteAll();
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::Accept()
{
  int i;
  float value;
  int numContours;
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }

  // Hit the add value button incase the user forgot.
  // This does nothing if there is no value in there.
  if (strcmp(this->NewValueEntry->GetValue(), "") != 0)
    {
    this->ContourValuesList->AppendUnique(this->NewValueEntry->GetValue());
    this->NewValueEntry->SetValue("");
    }
  numContours = this->ContourValuesList->GetNumberOfItems();

  if (this->ModifiedFlag)
    {  
    this->AddTraceEntry("$kw(%s) RemoveAllValues", 
                         this->GetTclName());
    pvApp->BroadcastScript("%s SetNumberOfContours %d",
                           this->PVSource->GetVTKSourceTclName(), numContours);
  
    for (i = 0; i < numContours; i++)
      {
      value = atof(this->ContourValuesList->GetItem(i));
      pvApp->BroadcastScript("%s SetValue %d %f",
                             this->PVSource->GetVTKSourceTclName(),
                             i, value);
      this->AddTraceEntry("$kw(%s) AddValue %f", 
                           this->GetTclName(), value);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVContourEntry::SaveInTclScript(ofstream *file)
{
  int i;
  float value;
  int numContours;

  numContours = this->ContourValuesList->GetNumberOfItems();

  for (i = 0; i < numContours; i++)
    {
    value = atof(this->ContourValuesList->GetItem(i));
    *file << "\t";
    *file << this->PVSource->GetVTKSourceTclName() << " SetValue " 
          << i << " " << value << endl;
    }
}

//----------------------------------------------------------------------------
// If we had access to the ContourValues object of the filter,
// this would be much easier.  We would not have to rely on Tcl calls.
void vtkPVContourEntry::Reset()
{
  int i;
  int numContours;
  
  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource not set.");
    return;
    }
  this->Script("%s GetNumberOfContours", 
               this->PVSource->GetVTKSourceTclName());
  numContours = this->GetIntegerResult(this->Application);

  // The widget has been modified.  
  // Now set the widget back to reflect the contours in the filter.
  this->ContourValuesList->DeleteAll();
  for (i = 0; i < numContours; i++)
    {
    this->Script("%s AppendUnique [%s GetValue %d]", 
                 this->ContourValuesList->GetTclName(),
                 this->PVSource->GetVTKSourceTclName(), i);
    }

  // Since the widget now matches the fitler, it is no longer modified.
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVContourEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                  vtkPVAnimationInterface *ai)
{
  char methodAndArgs[500];

  sprintf(methodAndArgs, "SetLabelAndScript {%s} {%s SetValue 0 $pvTime}", 
          this->GetTraceName(), this->PVSource->GetVTKSourceTclName());

  menu->AddCommand(this->GetTraceName(), ai, methodAndArgs, 0, "");
}

//----------------------------------------------------------------------------
vtkPVContourEntry* vtkPVContourEntry::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVContourEntry::SafeDownCast(clone);
}

void vtkPVContourEntry::CopyProperties(vtkPVWidget* clone, 
				       vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVContourEntry* pvce = vtkPVContourEntry::SafeDownCast(clone);
  if (pvce)
    {
    pvce->SetLabel(this->ContourValuesLabel->GetLabel());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVContourEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVContourEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                         vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->SetLabel(label);
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVContourEntry::GetLabel() 
{
  return this->ContourValuesLabel->GetLabel();
}
