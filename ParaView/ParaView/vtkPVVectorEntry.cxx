/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVVectorEntry.cxx
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
#include "vtkPVVectorEntry.h"

#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWWidgetCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterface.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"
#include "vtkStringList.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVectorEntry);

//---------------------------------------------------------------------------
vtkPVVectorEntry::vtkPVVectorEntry()
{
  this->LabelWidget  = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->Entries      = vtkKWWidgetCollection::New();
  this->SubLabels    = vtkKWWidgetCollection::New();

  this->ScriptValue  = NULL;
  this->DataType     = VTK_FLOAT;
  this->SubLabelTxts = vtkStringList::New();

  this->VectorLength = 1;
  this->EntryLabel   = 0;
  this->ReadOnly     = 0;

  int cc;
  for ( cc = 0; cc < 6; cc ++ )
    {
    this->EntryValues[cc] = 0;
    }
}

//---------------------------------------------------------------------------
vtkPVVectorEntry::~vtkPVVectorEntry()
{
  this->Entries->Delete();
  this->Entries = NULL;
  this->SubLabels->Delete();
  this->SubLabels = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;

  this->SetScriptValue(NULL);
  this->SubLabelTxts->Delete();
  this->SetEntryLabel(0);

  int cc;
  for ( cc = 0; cc < 6; cc ++ )
    {
    if ( this->EntryValues[cc] )
      {
      delete [] this->EntryValues[cc];
      this->EntryValues[cc] = 0;
      }
    }
}

void vtkPVVectorEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

void vtkPVVectorEntry::SetSubLabel(int i, const char* sublabel)
{
  this->SubLabelTxts->SetString(i, sublabel);
}

void vtkPVVectorEntry::SetBalloonHelpString(const char *str)
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
    this->LabelWidget->SetBalloonHelpString(this->BalloonHelpString);
    this->SubLabels->InitTraversal();
    int i, numItems = this->SubLabels->GetNumberOfItems();
    for (i=0; i<numItems; i++)
      {
      this->SubLabels->GetNextKWWidget()->SetBalloonHelpString(str);
      }
    this->Entries->InitTraversal();
    numItems = this->Entries->GetNumberOfItems();
    for (i=0; i<numItems; i++)
      {
      this->Entries->GetNextKWWidget()->SetBalloonHelpString(str);
      }
    this->BalloonHelpInitialized = 1;
    }
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  int i;
  vtkKWEntry* entry;
  vtkKWLabel* subLabel;
  
  if (this->Application)
    {
    vtkErrorMacro("VectorEntry already created");
    return;
    }
  
  // For getting the widget in a script.
  this->SetTraceName(this->EntryLabel);

  this->SetApplication(pvApp);

  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  if (this->EntryLabel && this->EntryLabel[0] != '\0')
    {
    this->LabelWidget->Create(pvApp, "-width 18 -justify right");
    this->LabelWidget->SetLabel(this->EntryLabel);
    this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
    }
    
  // Now the sublabels and entries
  char* subLabelTxt;
  for (i = 0; i < this->VectorLength; i++)
    {
    subLabelTxt = this->SubLabelTxts->GetString(i);
    if (subLabelTxt && subLabelTxt[0] != '\0')
      {
      subLabel = vtkKWLabel::New();
      subLabel->SetParent(this);
      subLabel->Create(pvApp, "");
      subLabel->SetLabel(subLabelTxt);
      this->Script("pack %s -side left", subLabel->GetWidgetName());
      this->SubLabels->AddItem(subLabel);
      subLabel->Delete();
      }
    
    entry = vtkKWEntry::New();
    entry->SetParent(this);
    entry->Create(pvApp, "-width 2");
    if ( this->ReadOnly ) 
      {
      entry->ReadOnlyOn();
      }
    else
      {
      this->Script("bind %s <KeyPress> {%s CheckModifiedCallback %K}",
                   entry->GetWidgetName(), this->GetTclName());
      this->Script("bind %s <FocusOut> {%s CheckModifiedCallback {}}",
                   entry->GetWidgetName(), this->GetTclName());
      }
    this->Script("pack %s -side left -fill x -expand t",
                 entry->GetWidgetName());
    
    this->Entries->AddItem(entry);
    entry->Delete();
    }
  this->SetBalloonHelpString(this->BalloonHelpString);
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::CheckModifiedCallback(const char* key)
{
  int found = 0;
  int cc;
  if ( vtkString::Equals(key, "Tab") ||
       vtkString::Equals(key, "ISO_Left_Tab") ||
       vtkString::Equals(key, "Return") ||
       vtkString::Equals(key, "") )
    {
    for (cc = 0; cc < this->Entries->GetNumberOfItems(); cc ++ )
      {
      const char* val = this->EntryValues[cc];
      if ( !vtkString::Equals(val, this->GetEntry(cc)->GetValue()) )
        {
        if ( this->EntryValues[cc] )
          {
          delete[] this->EntryValues[cc];
          }
        this->EntryValues[cc] = vtkString::Duplicate(this->GetEntry(cc)->GetValue());
        this->AcceptedCallback();
        this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent, 0);
        }
      }
    }
  else if ( vtkString::Equals(key, "Escape") )
    {
    for (cc = 0; cc < this->Entries->GetNumberOfItems(); cc ++ )
      {
      const char* val = this->EntryValues[cc];
      if ( !vtkString::Equals(val, this->GetEntry(cc)->GetValue()) )
        {
        this->GetEntry(cc)->SetValue(val);
        }
      }
    }
  else
    {
    found = 1;
    }
  if ( found )
    {
    this->ModifiedCallback();
    }
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::Accept()
{
  vtkKWEntry *entry;
  vtkPVApplication *pvApp = this->GetPVApplication();
  ofstream *traceFile = pvApp->GetTraceFile();
  int traceFlag = 0;
  char acceptCmd[1024];

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Start the trace entry and the accept command.
  if (traceFile && this->InitializeTrace())
    {
    traceFlag = 1;
    }

  if (traceFlag)
    {
    *traceFile << "$kw(" << this->GetTclName() << ") SetValue";
    }
  sprintf(acceptCmd, "%s Set%s ", this->ObjectTclName, this->VariableName);

  // finish all the arguments for the trace file and the accept command.
  this->Entries->InitTraversal();
  while ( (entry = (vtkKWEntry*)(this->Entries->GetNextItemAsObject())) )
    {
    if (traceFlag)
      {
      *traceFile << " " << entry->GetValue();
      }
    strcat(acceptCmd, entry->GetValue());
    strcat(acceptCmd, " ");
    }
  if (traceFlag)
    {
    *traceFile << endl;
    }
  pvApp->BroadcastScript(acceptCmd);

  this->ModifiedFlag = 0;  
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::Reset()
{
  int count = 0;

  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Set each entry to the appropriate value.
  for( count = 0; count < this->Entries->GetNumberOfItems(); count ++ )
    {
    this->Script("%s SetEntryValue %d [lindex [%s Get%s] %d]",
                 this->GetTclName(), count, this->ObjectTclName, 
                 this->VariableName, count); 
    }

  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetEntryValue(int index, const char* value)
{
  if ( index < 0 || index >= this->Entries->GetNumberOfItems() )
    {
    return;
    }
  this->GetEntry(index)->SetValue(value);
  if ( this->EntryValues[index] )
    {
    delete [] this->EntryValues[index];
    }
  this->EntryValues[index] = vtkString::Duplicate(value);
}

//---------------------------------------------------------------------------
vtkKWLabel* vtkPVVectorEntry::GetSubLabel(int idx)
{
  if (idx > this->SubLabels->GetNumberOfItems())
    {
    return NULL;
    }
  return ((vtkKWLabel*)this->SubLabels->GetItemAsObject(idx));
}

//---------------------------------------------------------------------------
vtkKWEntry* vtkPVVectorEntry::GetEntry(int idx)
{
  if (idx > this->Entries->GetNumberOfItems())
    {
    return NULL;
    }
  return ((vtkKWEntry*)this->Entries->GetItemAsObject(idx));
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char** values, int num)
{
  int idx;
  vtkKWEntry *entry;

  if (num != this->Entries->GetNumberOfItems())
    {
    vtkErrorMacro("Componenet mismatch.");
    return;
    }
  for (idx = 0; idx < num; ++idx)
    {
    entry = this->GetEntry(idx);    
    entry->SetValue(values[idx]);
    if ( this->EntryValues[idx] )
      {
      delete [] this->EntryValues[idx];
      }
    this->EntryValues[idx] = vtkString::Duplicate(values[idx]);
    }
  this->ModifiedCallback();
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(float* values, int num)
{
  int idx;
  vtkKWEntry *entry;

  if (num != this->Entries->GetNumberOfItems())
    {
    vtkErrorMacro("Componenet mismatch.");
    return;
    }
  for (idx = 0; idx < num; ++idx)
    {
    entry = this->GetEntry(idx);    
    entry->SetValue(values[idx], 4);
    if ( this->EntryValues[idx] )
      {
      delete [] this->EntryValues[idx];
      }
    this->EntryValues[idx] = vtkString::Duplicate(entry->GetValue());
    }
  this->ModifiedCallback();
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::GetValue(float *values, int num)
{
  int idx;
  vtkKWEntry *entry;

  if (num != this->Entries->GetNumberOfItems())
    {
    vtkErrorMacro("Componenet mismatch.");
    return;
    }
  for (idx = 0; idx < num; ++idx)
    {
    entry = this->GetEntry(idx);    
    values[idx] = atof(entry->GetValue());
    }
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0)
{
  char* vals[1];
  vals[0] = v0;
  this->SetValue(vals, 1);
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1)
{
  char* vals[2];
  vals[0] = v0;
  vals[1] = v1;
  this->SetValue(vals, 2);
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2)
{
  char* vals[3];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  this->SetValue(vals, 3);
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, char *v3)
{
  char* vals[4];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  this->SetValue(vals, 4);
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, char *v3, char *v4)
{
  char* vals[5];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  vals[4] = v4;
  this->SetValue(vals, 5);
}

//---------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, 
                                char *v3, char *v4, char *v5)
{
  char* vals[6];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  vals[4] = v4;
  vals[5] = v5;
  this->SetValue(vals, 6);
}

//----------------------------------------------------------------------------
void vtkPVVectorEntry::SaveInTclScript(ofstream *file)
{
  if (this->ScriptValue == NULL)
    {
    vtkPVObjectWidget::SaveInTclScript(file);
    return;
    }
  
  *file << "\t" << this->ObjectTclName << " Set" << this->VariableName;
  *file << " " << this->ScriptValue << "\n";
}

//----------------------------------------------------------------------------
void vtkPVVectorEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                                 vtkPVAnimationInterface *ai)
{
  char methodAndArgs[500];
  
  if (this->Entries->GetNumberOfItems() == 1)
    {
    // I do not like setting the label like this but ...
    if (this->DataType == VTK_INT || this->DataType == VTK_LONG)
      {
      sprintf(methodAndArgs, "SetLabelAndScript {%s} {%s Set%s [expr int($pvTime)]}", 
              this->LabelWidget->GetLabel(), this->ObjectTclName, 
              this->VariableName);
      }
    else
      {
      sprintf(methodAndArgs, "SetLabelAndScript {%s} {%s Set%s $pvTime}", 
              this->LabelWidget->GetLabel(), this->ObjectTclName, 
              this->VariableName);
      }
    menu->AddCommand(this->LabelWidget->GetLabel(), ai, methodAndArgs, 0, "");
    }
}

//----------------------------------------------------------------------------
void vtkPVVectorEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DataType: " << this->GetDataType() << endl;
  os << indent << "Entries: " << this->GetEntries() << endl;
  os << indent << "ScriptValue: " << (this->ScriptValue?this->ScriptValue:"none") << endl;
  os << indent << "SubLabels: " << this->GetSubLabels() << endl;
  os << indent << "LabelWidget: " << this->LabelWidget << endl;
  os << indent << "ReadOnly: " << this->ReadOnly << endl;
  os << indent << "VectorLength: " << this->VectorLength << endl;
}

vtkPVVectorEntry* vtkPVVectorEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVVectorEntry::SafeDownCast(clone);
}

void vtkPVVectorEntry::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVVectorEntry* pvve = vtkPVVectorEntry::SafeDownCast(clone);
  if (pvve)
    {
    pvve->SetLabel(this->EntryLabel);
    pvve->SetDataType(this->DataType);
    pvve->SetVectorLength(this->VectorLength);
    pvve->SetReadOnly(this->ReadOnly);
    int i, len = this->SubLabelTxts->GetLength();
    for (i=0; i<len; i++)
      {
      pvve->SubLabelTxts->SetString(i, this->SubLabelTxts->GetString(i));
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVVectorEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVVectorEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the VectorLength.
  if(!element->GetScalarAttribute("length", &this->VectorLength))
    {
    this->VectorLength = 1;
    }

  // Setup the VectorLength.
  if(!element->GetScalarAttribute("readonly", &this->ReadOnly))
    {
    this->ReadOnly = 0;
    }
  
  // Setup the DataType.
  const char* type = element->GetAttribute("type");
  if(!type)
    {
    vtkErrorMacro("No type attribute.");
    return 0;
    }
  if(strcmp(type, "int") == 0) { this->DataType = VTK_INT; }
  else if(strcmp(type, "float") == 0) { this->DataType = VTK_FLOAT; }
  else
    {
    vtkErrorMacro("Unknown type " << type);
    return 0;
    }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->VariableName);
    }
  
  // Setup the SubLabels.
  const char* sub_labels = element->GetAttribute("sub_labels");
  if(sub_labels)
    {
    const char* start = sub_labels;
    const char* end = 0;
    int index = 0;
    
    // Parse the semi-colon-separated list.
    while(*start)
      {
      while(*start && (*start == ';')) { ++start; }
      end = start;
      while(*end && (*end != ';')) { ++end; }
      int length = end-start;
      if(length)
        {
        char* entry = new char[length+1];
        strncpy(entry, start, length);
        entry[length] = '\0';
        this->SubLabelTxts->SetString(index++, entry);
        delete [] entry;
        }
      start = end;
      }
    }
  
  return 1;
}
