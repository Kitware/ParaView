/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVectorEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVVectorEntry.h"

#include "vtkKWEntry.h"
#include "vtkKWEvent.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWWidgetCollection.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkString.h"
#include "vtkStringList.h"


//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVectorEntry);
vtkCxxRevisionMacro(vtkPVVectorEntry, "1.55");

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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
  
  this->SetProperty(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetSubLabel(int i, const char* sublabel)
{
  this->SubLabelTxts->SetString(i, sublabel);
}

//-----------------------------------------------------------------------------
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

  if ( this->GetApplication() && !this->BalloonHelpInitialized )
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

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  int i;

  vtkKWEntry* entry;
  vtkKWLabel* subLabel;

  // For getting the widget in a script.

  if (this->EntryLabel && this->EntryLabel[0] &&
    (this->TraceNameState == vtkPVWidget::Uninitialized ||
     this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->EntryLabel);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  // Now a label
  if (this->EntryLabel && this->EntryLabel[0] != '\0')
    {
    this->LabelWidget->Create(pvApp, "-width 18 -justify right");
    this->LabelWidget->SetLabel(this->EntryLabel);
    this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
    }

  // Now the sublabels and entries
  const char* subLabelTxt;
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::Accept()
{
  int modFlag = this->GetModifiedFlag();
  int i;
  vtkKWEntry *entry;

  this->Entries->InitTraversal();
  
  switch (this->DataType)
    {
    case VTK_FLOAT:
    case VTK_DOUBLE:
      {
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->GetSMProperty());
      if (dvp)
        {
        dvp->SetNumberOfElements(this->VectorLength);
        for (i = 0; i < this->VectorLength; i++)
          {
          entry =
            vtkKWEntry::SafeDownCast(this->Entries->GetNextItemAsObject());
          dvp->SetElement(i, entry->GetValueAsFloat());
          }
        }
      break;
      }
    case VTK_INT:
      {
      vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
        this->GetSMProperty());
      if (ivp)
        {
        ivp->SetNumberOfElements(this->VectorLength);
        for (i = 0; i < this->VectorLength; i++)
          {
          entry =
            vtkKWEntry::SafeDownCast(this->Entries->GetNextItemAsObject());
          ivp->SetElement(i, entry->GetValueAsInt());
          }
        }
      break;
      }
    }
  
  this->ModifiedFlag = 0;
  
  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::Trace(ofstream *file)
{
  vtkKWEntry *entry;

  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue";

  // finish all the arguments for the trace file and the accept command.
  this->Entries->InitTraversal();
  while ( (entry = (vtkKWEntry*)(this->Entries->GetNextItemAsObject())) )
    {
    *file << " " << entry->GetValue();
    }
  *file << endl;
}



//-----------------------------------------------------------------------------
void vtkPVVectorEntry::ResetInternal()
{
  int i;
  
  switch (this->DataType)
    {
    case VTK_DOUBLE:
    case VTK_FLOAT:
      {
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->GetSMProperty());
      if (dvp)
        {
        for (i = 0; i < this->VectorLength; i++)
          {
          ostrstream val;
          val << dvp->GetElement(i) << ends;
          this->SetEntryValue(i, val.str());
          val.rdbuf()->freeze(0);
          }
        }
      break;
      }
    case VTK_INT:
      {
      vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
        this->GetSMProperty());
      if (ivp)
        {
        for (i = 0; i < this->VectorLength; i++)
          {
          ostrstream val;
          val << ivp->GetElement(i) << ends;
          this->SetEntryValue(i, val.str());
          val.rdbuf()->freeze(0);
          }
        }
      break;
      }
    }
  
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
vtkKWLabel* vtkPVVectorEntry::GetSubLabel(int idx)
{
  if (idx > this->SubLabels->GetNumberOfItems())
    {
    return NULL;
    }
  return ((vtkKWLabel*)this->SubLabels->GetItemAsObject(idx));
}

//-----------------------------------------------------------------------------
vtkKWEntry* vtkPVVectorEntry::GetEntry(int idx)
{
  if (idx > this->Entries->GetNumberOfItems())
    {
    return NULL;
    }
  return ((vtkKWEntry*)this->Entries->GetItemAsObject(idx));
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char** values, int num)
{
  int idx;
  vtkKWEntry *entry;

  if (num != this->Entries->GetNumberOfItems())
    {
    vtkErrorMacro("Componenet mismatch.");
    return;
    }

  float scalars[6];
  
  for (idx = 0; idx < num; ++idx)
    {
    entry = this->GetEntry(idx);    
    entry->SetValue(values[idx]);
    if ( this->EntryValues[idx] )
      {
      delete [] this->EntryValues[idx];
      }
    this->EntryValues[idx] = vtkString::Duplicate(values[idx]);
    sscanf(values[idx], "%f", &scalars[idx]);
    }
  
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(float* values, int num)
{
  int idx;
  vtkKWEntry *entry;

  if (num != this->Entries->GetNumberOfItems())
    {
    vtkErrorMacro("Componenet mismatch.");
    return;
    }
  
  float scalars[6];
  
  for (idx = 0; idx < num; ++idx)
    {
    entry = this->GetEntry(idx);    
    entry->SetValue(values[idx]);
    if ( this->EntryValues[idx] )
      {
      delete [] this->EntryValues[idx];
      }
    this->EntryValues[idx] = vtkString::Duplicate(entry->GetValue());
    scalars[idx] = entry->GetValueAsFloat();
    }
  
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0)
{
  char* vals[1];
  vals[0] = v0;
  this->SetValue(vals, 1);
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1)
{
  char* vals[2];
  vals[0] = v0;
  vals[1] = v1;
  this->SetValue(vals, 2);
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2)
{
  char* vals[3];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  this->SetValue(vals, 3);
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetValue(char *v0, char *v1, char *v2, char *v3)
{
  char* vals[4];
  vals[0] = v0;
  vals[1] = v1;
  vals[2] = v2;
  vals[3] = v3;
  this->SetValue(vals, 4);
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SaveInBatchScript(ofstream *file)
{
  int cc;
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  for ( cc = 0; cc < this->VectorLength; cc ++ )
    {
    *file << "  [$pvTemp" << sourceID <<  " GetProperty "
          << this->SMPropertyName << "] SetElement "
          << cc << " ";
    if (this->DataType == VTK_INT)
      {
      *file << "[expr round(" << this->EntryValues[cc] << ")]";
      }
    else
      {
      *file << this->EntryValues[cc];
      }
    *file << endl;
    }
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
  vtkPVAnimationInterfaceEntry *ai)
{
  char methodAndArgs[500];

  if (this->Entries->GetNumberOfItems() == 1)
    {
    sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
    menu->AddCommand(this->LabelWidget->GetLabel(), this, methodAndArgs, 0,"");
    }
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)",
                        this->GetTclName(), ai->GetTclName());
    }
  
  if (this->Entries->GetNumberOfItems() == 1)
    {
    ai->SetLabelAndScript(this->LabelWidget->GetLabel(), NULL, this->GetTraceName());
    vtkSMProperty *prop = this->GetSMProperty();
    vtkSMDomain *rangeDomain = prop->GetDomain("range");
    
    ai->SetCurrentSMProperty(prop);
    ai->SetCurrentSMDomain(rangeDomain);
    ai->SetAnimationElement(0);

    if (rangeDomain)
      {
      vtkSMIntRangeDomain *intRangeDomain =
        vtkSMIntRangeDomain::SafeDownCast(rangeDomain);
      vtkSMDoubleRangeDomain *doubleRangeDomain =
        vtkSMDoubleRangeDomain::SafeDownCast(rangeDomain);
      int minExists = 0, maxExists = 0;
      if (intRangeDomain)
        {
        int min = intRangeDomain->GetMinimum(0, minExists);
        int max = intRangeDomain->GetMaximum(0, maxExists);
        if (minExists && maxExists)
          {
          ai->SetTimeStart(min);
          ai->SetTimeEnd(max);
          }
        }
      else if (doubleRangeDomain)
        {
        double min = doubleRangeDomain->GetMinimum(0, minExists);
        double max = doubleRangeDomain->GetMaximum(0, maxExists);
        if (minExists && maxExists)
          {
          ai->SetTimeStart(min);
          ai->SetTimeEnd(max);
          }
        }
      }
    ai->Update();
    }
    // What if there are more than one entry?
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DataType: " << this->GetDataType() << endl;
  os << indent << "Entries: " << this->GetEntries() << endl;
  os << indent << "ScriptValue: " 
    << (this->ScriptValue?this->ScriptValue:"none") << endl;
  os << indent << "SubLabels: " << this->GetSubLabels() << endl;
  os << indent << "LabelWidget: " << this->LabelWidget << endl;
  os << indent << "ReadOnly: " << this->ReadOnly << endl;
  os << indent << "VectorLength: " << this->VectorLength << endl;
}

//-----------------------------------------------------------------------------
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
    pvve->SetUseWidgetRange(this->UseWidgetRange);
    pvve->SetWidgetRange(this->WidgetRange);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVVectorEntry.");
    }
}

//-----------------------------------------------------------------------------
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

  const char *range = element->GetAttribute("data_range");
  if (range)
    {
    sscanf(range, "%lf %lf", &this->WidgetRange[0], &this->WidgetRange[1]);
    this->UseWidgetRange = 1;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVVectorEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  if ( this->LabelWidget )
    {
    this->LabelWidget->SetEnabled(this->Enabled);
    }
  int cc;
  for ( cc = 0; cc < this->VectorLength; cc ++ )
    {
    if ( this->SubLabels )
      {
      vtkKWWidget* w = vtkKWWidget::SafeDownCast(this->SubLabels->GetItemAsObject(cc));
      if ( w )
        {
        w->SetEnabled(this->Enabled);
        }
      }
    if ( this->Entries )
      {
      vtkKWWidget* w = vtkKWWidget::SafeDownCast(this->Entries->GetItemAsObject(cc));
      if ( w )
        {
        w->SetEnabled(this->Enabled);
        }
      }
    }
}

