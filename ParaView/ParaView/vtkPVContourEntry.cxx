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
#include "vtkContourValues.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVScalarRangeLabel.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVXMLElement.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVContourEntry);
vtkCxxRevisionMacro(vtkPVContourEntry, "1.31");

vtkCxxSetObjectMacro(vtkPVContourEntry, ScalarRangeLabel,
                     vtkPVScalarRangeLabel);

int vtkPVContourEntryCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//-----------------------------------------------------------------------------
vtkPVContourEntry::vtkPVContourEntry()
{
  this->CommandFunction = vtkPVContourEntryCommand;

  this->ContourValues = vtkContourValues::New();  
  this->ContourValuesLabel = vtkKWLabel::New();
  this->ContourValuesList = vtkKWListBox::New();
  this->NewValueFrame = vtkKWWidget::New();
  this->NewValueLabel = vtkKWLabel::New();
  this->NewValueEntry = vtkKWEntry::New();
  this->AddValueButton = vtkKWPushButton::New();
  this->DeleteValueButton = vtkKWPushButton::New();
  this->GenerateFrame = vtkKWWidget::New();
  this->GenerateLabel = vtkKWLabel::New();
  this->GenerateEntry = vtkKWEntry::New();
  this->GenerateButton = vtkKWPushButton::New();
  this->GenerateRangeFrame = vtkKWWidget::New();
  this->GenerateRangeLabel = vtkKWLabel::New();
  this->GenerateRangeMinEntry = vtkKWEntry::New();
  this->GenerateRangeMaxEntry = vtkKWEntry::New();
  
  this->ContourValues->SetNumberOfContours(0);
  this->SuppressReset = 1;
  
  this->ScalarRangeLabel = NULL;
}

//-----------------------------------------------------------------------------
vtkPVContourEntry::~vtkPVContourEntry()
{
  this->ContourValues->Delete();
  this->ContourValues = NULL;
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
  this->GenerateFrame->Delete();
  this->GenerateFrame = NULL;
  this->GenerateLabel->Delete();
  this->GenerateLabel = NULL;
  this->GenerateEntry->Delete();
  this->GenerateEntry = NULL;
  this->GenerateButton->Delete();
  this->GenerateButton = NULL;
  this->GenerateRangeFrame->Delete();
  this->GenerateRangeFrame = NULL;
  this->GenerateRangeLabel->Delete();
  this->GenerateRangeLabel = NULL;
  this->GenerateRangeMinEntry->Delete();
  this->GenerateRangeMinEntry = NULL;
  this->GenerateRangeMaxEntry->Delete();
  this->GenerateRangeMaxEntry = NULL;
  
  this->SetPVSource(NULL);
  this->SetScalarRangeLabel(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::SetLabel(const char* str)
{
  this->ContourValuesLabel->SetLabel(str);
  if (str && str[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(str);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//-----------------------------------------------------------------------------
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
  this->NewValueLabel->SetBalloonHelpString("Enter a new value");
  
  this->NewValueEntry->SetParent(this->NewValueFrame);
  this->NewValueEntry->Create(app, "");
  this->NewValueEntry->SetValue("");
  this->NewValueEntry->SetBalloonHelpString("Enter a new value");
  this->Script("bind %s <KeyPress-Return> {%s AddValueCallback}",
               this->NewValueEntry->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->NewValueEntry->GetWidgetName(), this->GetTclName());
  
  this->AddValueButton->SetParent(this->NewValueFrame);
  this->AddValueButton->Create(app, "-text {Add}");
  this->AddValueButton->SetCommand(this, "AddValueCallback");
  this->AddValueButton->SetBalloonHelpString("Add the new value to the list");
  
  this->Script("pack %s %s %s -side left",
               this->NewValueLabel->GetWidgetName(),
               this->NewValueEntry->GetWidgetName(),
               this->AddValueButton->GetWidgetName());
  
  this->DeleteValueButton->SetParent(this->NewValueFrame);
  this->DeleteValueButton->Create(app, "-text {Delete}");
  this->DeleteValueButton->SetCommand(this, "DeleteValueCallback");
  this->DeleteValueButton->SetBalloonHelpString(
    "Remove the currently selected  value from the list");

  this->Script("pack %s -side left",
               this->DeleteValueButton->GetWidgetName());

  this->SetBalloonHelpString(this->BalloonHelpString);

  char *className = this->PVSource->GetSourceClassName();
  if (strcmp(className, "vtkPVKitwareContourFilter") != 0 ||
      strcmp(className, "vtkPVContourFilter") != 0)
    {
    this->GenerateFrame->SetParent(this);
    this->GenerateFrame->Create(app, "frame", "");

    this->GenerateRangeFrame->SetParent(this);
    this->GenerateRangeFrame->Create(app, "frame", "");

    this->Script("pack %s %s", this->GenerateFrame->GetWidgetName(),
                 this->GenerateRangeFrame->GetWidgetName());
    
    this->GenerateLabel->SetParent(this->GenerateFrame);
    this->GenerateLabel->Create(app, "");
    this->GenerateLabel->SetLabel("Number of Values to Generate:");
    
    this->GenerateEntry->SetParent(this->GenerateFrame);
    this->GenerateEntry->Create(app, "");
    this->GenerateEntry->SetValue("");
    this->Script("bind %s <KeyPress-Return> {%s GenerateValuesCallback}",
                 this->GenerateEntry->GetWidgetName(), this->GetTclName());
    
    this->GenerateButton->SetParent(this->GenerateFrame);
    this->GenerateButton->Create(app, "");
    this->GenerateButton->SetLabel("Generate");
    this->GenerateButton->SetCommand(this, "GenerateValuesCallback");
    
    this->Script("pack %s %s %s -side left",
                 this->GenerateLabel->GetWidgetName(),
                 this->GenerateEntry->GetWidgetName(),
                 this->GenerateButton->GetWidgetName());
    
    this->GenerateRangeLabel->SetParent(this->GenerateRangeFrame);
    this->GenerateRangeLabel->SetLabel("Range for Generated Values:");
    this->GenerateRangeLabel->Create(app, "");
    
    this->GenerateRangeMinEntry->SetParent(this->GenerateRangeFrame);
    this->GenerateRangeMinEntry->SetWidth(13);
    this->GenerateRangeMinEntry->Create(app, "");
    
    this->GenerateRangeMaxEntry->SetParent(this->GenerateRangeFrame);
    this->GenerateRangeMaxEntry->SetWidth(13);
    this->GenerateRangeMaxEntry->Create(app, "");
    
    this->Script("pack %s %s %s -side left",
                 this->GenerateRangeLabel->GetWidgetName(),
                 this->GenerateRangeMinEntry->GetWidgetName(),
                 this->GenerateRangeMaxEntry->GetWidgetName());
    }
  
  // Get the default values in the UI.
  this->Update();
}



//-----------------------------------------------------------------------------
void vtkPVContourEntry::Update()
{
  int num, idx;
  char str[256];

  if (this->Application == NULL)
    {
    return;
    }

  this->ContourValuesList->DeleteAll();
  num = this->ContourValues->GetNumberOfContours();
  for (idx = 0; idx < num; ++idx)
    {
    sprintf(str, "%g", this->ContourValues->GetValue(idx));
    this->ContourValuesList->AppendUnique(str);
    }    
}



//-----------------------------------------------------------------------------
void vtkPVContourEntry::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->ContourValuesList->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AddValueCallback()
{
  if (strcmp(this->NewValueEntry->GetValue(), "") == 0)
    {
    return;
    }

  this->ContourValues->SetValue(this->ContourValues->GetNumberOfContours(),
                                this->NewValueEntry->GetValueAsFloat());
  this->Update();
  this->NewValueEntry->SetValue("");
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::DeleteValueCallback()
{
  int index;
  int num, idx;
  
  num = this->ContourValues->GetNumberOfContours();

  // First look for selected values in the value list.
  index = this->ContourValuesList->GetSelectionIndex();
  if (index == -1)
    {
    // Next look for values in the entry box.
    if (strcmp(this->NewValueEntry->GetValue(), "") != 0)
      {
      // Find the index of the value in the entry box.
      // If the entry value is not in the list,
      // this will just clear the entry and return.
      for (idx = 0; idx < num && index < 0; ++idx)
        {
        if (this->NewValueEntry->GetValueAsFloat() == 
              this->ContourValues->GetValue(idx))
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
    for (idx = index+1; idx < num; ++idx)
      {
      this->ContourValues->SetValue(idx-1,
                              this->ContourValues->GetValue(idx));
      }
    this->ContourValues->SetNumberOfContours(num-1);
    this->Update();
    this->ModifiedCallback();
    }

  this->NewValueEntry->SetValue("");
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::GenerateValuesCallback()
{
  if (!this->ScalarRangeLabel)
    {
    return;
    }
  
  double range[2];
  range[0] = this->GenerateRangeMinEntry->GetValueAsFloat();
  range[1] = this->GenerateRangeMaxEntry->GetValueAsFloat();

  if (range[0] == 0 && range[1] == 0) // happens if the entries are empty
    {
    this->ScalarRangeLabel->GetRange(range);
    }

  int numContours = this->GenerateEntry->GetValueAsInt();
  double step = (range[1] - range[0]) / (float)(numContours-1);
  
  int i;
  for (i = 0; i < numContours; i++)
    {
    this->AddValue(i*step+range[0]);
    }
  this->Update();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AddValueInternal(float val)
{
  int num = this->ContourValues->GetNumberOfContours();

  this->ContourValues->SetValue(num, val);
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AddValue(float val)
{
  this->AddValueInternal(val);
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::RemoveAllValues()
{
  this->ContourValues->SetNumberOfContours(0);
  this->Update();
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::AcceptInternal(const char* sourceTclName)
{
  int i;
  float value;
  int numContours;
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (sourceTclName == NULL)
    {
    return;
    }

  // Hit the add value button incase the user forgot.
  // This does nothing if there is no value in there.
  if (strcmp(this->NewValueEntry->GetValue(), "") != 0)
    {
    this->ContourValues->SetValue(this->ContourValues->GetNumberOfContours(),
                                  this->NewValueEntry->GetValueAsFloat());
    this->Update();
    this->NewValueEntry->SetValue("");
    }

  numContours = this->ContourValues->GetNumberOfContours();

  pvApp->BroadcastScript("%s SetNumberOfContours %d",
                         sourceTclName, numContours);
  
  for (i = 0; i < numContours; i++)
    {
    value = this->ContourValues->GetValue(i);
    pvApp->BroadcastScript("%s SetValue %d %f",
                           sourceTclName, i, value);
    }
}


//-----------------------------------------------------------------------------
void vtkPVContourEntry::Trace(ofstream *file)
{
  int i, numContours;
  float value;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") RemoveAllValues \n";

  numContours = this->ContourValues->GetNumberOfContours();
  for (i = 0; i < numContours; i++)
    {
    value = this->ContourValues->GetValue(i);
    *file << "$kw(" << this->GetTclName() << ") AddValue "
          << value << endl;
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
void vtkPVContourEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                  vtkPVAnimationInterfaceEntry *ai)
{
  char methodAndArgs[500];
  
  sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
  menu->AddCommand(this->GetTraceName(), this, methodAndArgs, 0,"");
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

//-----------------------------------------------------------------------------
vtkPVContourEntry* vtkPVContourEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVContourEntry::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::CopyProperties(vtkPVWidget* clone, 
                                       vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  int idx, num;

  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVContourEntry* pvce = vtkPVContourEntry::SafeDownCast(clone);
  if (pvce)
    {
    pvce->SetLabel(this->ContourValuesLabel->GetLabel());
    num = this->ContourValues->GetNumberOfContours();
    for (idx = 0; idx < num; ++idx)
      {
      pvce->AddValue(this->ContourValues->GetValue(idx));
      }
    if (this->ScalarRangeLabel)
      {
      vtkPVScalarRangeLabel *srl =
        this->ScalarRangeLabel->ClonePrototype(pvSource, map);
      pvce->SetScalarRangeLabel(srl);
      srl->Delete();
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
  
  const char* attr;

  attr = element->GetAttribute("label");
  if(!attr)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->SetLabel(attr);
  
  attr = element->GetAttribute("initial_value");
  if(attr)
    {
    this->AddValue(atof(attr));
    }

  const char* scalar_range = element->GetAttribute("scalar_range");
  if (scalar_range)
    {
    vtkPVXMLElement *ime = element->LookupElement(scalar_range);
    vtkPVWidget *w = this->GetPVWidgetFromParser(ime, parser);
    vtkPVScalarRangeLabel *srl = vtkPVScalarRangeLabel::SafeDownCast(w);
    if (!srl)
      {
      if (w)
        {
        w->Delete();
        }
      vtkErrorMacro("Couldn't get ScalarRangeLabel " << scalar_range);
      return 0;
      }
    srl->AddDependent(this);
    this->SetScalarRangeLabel(srl);
    srl->Delete();
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
const char* vtkPVContourEntry::GetLabel() 
{
  return this->ContourValuesLabel->GetLabel();
}

//-----------------------------------------------------------------------------
void vtkPVContourEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
