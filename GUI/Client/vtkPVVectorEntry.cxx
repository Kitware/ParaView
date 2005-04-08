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
#include "vtkCommand.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
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
#include "vtkStringList.h"
#include "vtkPVTraceHelper.h"

#include <kwsys/SystemTools.hxx>

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVVectorEntry);
vtkCxxRevisionMacro(vtkPVVectorEntry, "1.73");

//-----------------------------------------------------------------------------
vtkPVVectorEntry::vtkPVVectorEntry()
{
  this->LabelWidget  = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->Entries      = vtkKWWidgetCollection::New();

  this->ScriptValue  = NULL;
  this->DataType     = VTK_FLOAT;

  this->VectorLength = 1;
  this->EntryLabel   = 0;

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
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;

  this->SetScriptValue(NULL);
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

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetText(label);
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::SetBalloonHelpString(const char *str)
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->LabelWidget)
    {
    this->LabelWidget->SetBalloonHelpString(str);
    }

  if (this->Entries)
    {
    this->Entries->InitTraversal();
    int numItems = this->Entries->GetNumberOfItems();
    int i;
    for (i=0; i<numItems; i++)
      {
      this->Entries->GetNextKWWidget()->SetBalloonHelpString(str);
      }
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

  // For getting the widget in a script.

  if (this->EntryLabel && this->EntryLabel[0] &&
    (this->GetTraceHelper()->GetObjectNameState() == 
     vtkPVTraceHelper::ObjectNameStateUninitialized ||
     this->GetTraceHelper()->GetObjectNameState() == 
     vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName(this->EntryLabel);
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }

  // Now a label
  if (this->EntryLabel && this->EntryLabel[0] != '\0')
    {
    this->LabelWidget->Create(pvApp, "-width 18 -justify right");
    this->LabelWidget->SetText(this->EntryLabel);
    this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());
    }

  // Now the entries
  for (i = 0; i < this->VectorLength; i++)
    {
    entry = vtkKWEntry::New();
    entry->SetParent(this);
    entry->Create(pvApp, "-width 2");
    this->Script("bind %s <KeyPress> {%s CheckModifiedCallback %K}",
                 entry->GetWidgetName(), this->GetTclName());
    this->Script("bind %s <FocusOut> {%s CheckModifiedCallback {}}",
                 entry->GetWidgetName(), this->GetTclName());
    this->Script("pack %s -side left -fill x -expand t",
      entry->GetWidgetName());

    this->Entries->AddItem(entry);
    entry->Delete();
    }
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::CheckModifiedCallback(const char* key)
{
  int found = 0;
  int cc;
  if (key && (!strcmp(key, "Tab") ||
              !strcmp(key, "ISO_Left_Tab") ||
              !strcmp(key, "Return") ||
              !strcmp(key, "")))
    {
    for (cc = 0; cc < this->Entries->GetNumberOfItems(); cc ++ )
      {
      const char* val = this->EntryValues[cc];
      if (!val || (this->GetEntry(cc)->GetValue() && 
                   strcmp(val, this->GetEntry(cc)->GetValue())))
        {
        if ( this->EntryValues[cc] )
          {
          delete[] this->EntryValues[cc];
          }
        this->EntryValues[cc] = kwsys::SystemTools::DuplicateString(
          this->GetEntry(cc)->GetValue());
        this->AcceptedCallback();
        this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
        }
      }
    }
  else if (key && !strcmp(key, "Escape") )
    {
    for (cc = 0; cc < this->Entries->GetNumberOfItems(); cc ++ )
      {
      const char* val = this->EntryValues[cc];
      if (!val || (this->GetEntry(cc)->GetValue() && 
                   strcmp(val, this->GetEntry(cc)->GetValue())))
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
  int i;
  vtkKWEntry *entry;

  this->Entries->InitTraversal();

  int propFound = 0;
  switch (this->DataType)
    {
    case VTK_FLOAT:
    case VTK_DOUBLE:
      {
      vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
        this->GetSMProperty());
      if (dvp)
        {
        propFound = 1;
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
        propFound = 1;
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

  if (!propFound)
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    }

  this->Superclass::Accept();
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::Trace(ofstream *file)
{
  vtkKWEntry *entry;

  if ( ! this->GetTraceHelper()->Initialize(file))
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
void vtkPVVectorEntry::Initialize()
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
  
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::ResetInternal()
{
  if (!this->ModifiedFlag)
    {
    return;
    }
  this->Initialize();
  this->ModifiedFlag = 0;
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
  this->EntryValues[index] = kwsys::SystemTools::DuplicateString(value);
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
    this->EntryValues[idx] = kwsys::SystemTools::DuplicateString(values[idx]);
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
    this->EntryValues[idx] = 
      kwsys::SystemTools::DuplicateString(entry->GetValue());
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
  char command[200];
  
  if (this->Entries->GetNumberOfItems() == 1)
    {
    sprintf(methodAndArgs, "AnimationMenuCallback %s 0", ai->GetTclName()); 
    menu->AddCommand(this->LabelWidget->GetText(), this, methodAndArgs, 0,"");
    }
  else
    {
    vtkKWMenu *cascadeMenu = vtkKWMenu::New();
    cascadeMenu->SetParent(menu);
    cascadeMenu->Create(this->GetApplication(), "-tearoff 0");
    menu->AddCascade(this->GetTraceHelper()->GetObjectName(), cascadeMenu, 0,
                     "Choose a vector component to animate.");
    int i;
    for (i = 0; i < this->Entries->GetNumberOfItems(); i++)
      {
      sprintf(command, "Component %d", i);
      sprintf(methodAndArgs, "AnimationMenuCallback %s %d",
              ai->GetTclName(), i);
      cascadeMenu->AddCommand(command, this, methodAndArgs, 0, "");
      }
    cascadeMenu->Delete();
    cascadeMenu = NULL;
    }
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::ResetAnimationRange(
  vtkPVAnimationInterfaceEntry *ai, int idx)
{
  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDomain *rangeDomain = prop->GetDomain("range");

  if (rangeDomain)
    {
    vtkSMIntRangeDomain *intRangeDomain =
      vtkSMIntRangeDomain::SafeDownCast(rangeDomain);
    vtkSMDoubleRangeDomain *doubleRangeDomain =
      vtkSMDoubleRangeDomain::SafeDownCast(rangeDomain);
    int minExists = 0, maxExists = 0;
    if (intRangeDomain)
      {
      int min = intRangeDomain->GetMinimum(idx, minExists);
      int max = intRangeDomain->GetMaximum(idx, maxExists);
      if (minExists)
        {
        ai->SetTimeStart(min);
        }
      if (maxExists)
        {
        ai->SetTimeEnd(max);
        }
      }
    else if (doubleRangeDomain)
      {
      double min = doubleRangeDomain->GetMinimum(idx, minExists);
      double max = doubleRangeDomain->GetMaximum(idx, maxExists);
      if (minExists)
        {
        ai->SetTimeStart(min);
        }
      if (maxExists)
        {
        ai->SetTimeEnd(max);
        }
      }
    }
  else
    {
    vtkErrorMacro("Could not find required domain (range)");
    }
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai,
                                             int idx)
{
  if (ai->GetTraceHelper()->Initialize())
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) AnimationMenuCallback $kw(%s)",
                        this->GetTclName(), ai->GetTclName());
    }

  this->Superclass::AnimationMenuCallback(ai);

  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDomain *rangeDomain = prop->GetDomain("range");

  int enableReset = 0;
  if (rangeDomain)
    {
    vtkSMIntRangeDomain *intRangeDomain =
      vtkSMIntRangeDomain::SafeDownCast(rangeDomain);
    vtkSMDoubleRangeDomain *doubleRangeDomain =
      vtkSMDoubleRangeDomain::SafeDownCast(rangeDomain);
    int minExists = 0, maxExists = 0;
    if (intRangeDomain)
      {
      intRangeDomain->GetMinimum(idx, minExists);
      intRangeDomain->GetMaximum(idx, maxExists);
      if (minExists || maxExists)
        {
        enableReset = 1;
        }
      }
    else if (doubleRangeDomain)
      {
      doubleRangeDomain->GetMinimum(idx, minExists);
      doubleRangeDomain->GetMaximum(idx, maxExists);
      if (minExists || maxExists)
        {
        enableReset = 1;
        }
      }
    }

  if (enableReset)
    {
    char methodAndArgs[500];
    
    sprintf(methodAndArgs, "ResetAnimationRange %s %d", ai->GetTclName(), idx);
    ai->GetResetRangeButton()->SetCommand(this, methodAndArgs);
    ai->SetResetRangeButtonState(1);
    ai->UpdateEnableState();
    }

  if (this->Entries->GetNumberOfItems() == 1)
    {
    ai->SetLabelAndScript(this->LabelWidget->GetText(), NULL,
                          this->GetTraceHelper()->GetObjectName());
    }
  else
    {
    char label[200];
    sprintf(label, "%s (%d)", this->LabelWidget->GetText(), idx);
    ai->SetLabelAndScript(label, NULL, this->GetTraceHelper()->GetObjectName());
    }
  

  ai->SetCurrentSMProperty(prop);
  ai->SetCurrentSMDomain(rangeDomain);
  ai->SetAnimationElement(idx);

  this->ResetAnimationRange(ai, idx);
  ai->Update();
}

//-----------------------------------------------------------------------------
void vtkPVVectorEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "DataType: " << this->GetDataType() << endl;
  os << indent << "Entries: " << this->GetEntries() << endl;
  os << indent << "ScriptValue: " 
    << (this->ScriptValue?this->ScriptValue:"none") << endl;
  os << indent << "LabelWidget: " << this->LabelWidget << endl;
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

  // Setup the DataType.
  const char* type = element->GetAttribute("type");
  if(!type)
    {
    // I should not have to set the type of a scale factor entry (a subclass
    // of vtkPVVectorEntry) because the only type it supports is float.
    // Besides DataType is already initialized to this value in the
    // constructor.
    this->DataType = VTK_FLOAT;
    }
  else
    {
    if(strcmp(type, "int") == 0) { this->DataType = VTK_INT; }
    else if(strcmp(type, "float") == 0) { this->DataType = VTK_FLOAT; }
    else
      {
      vtkErrorMacro("Unknown type " << type);
      return 0;
      }
    }

  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->SetLabel(label);
    }
  else
    {
    this->SetLabel(this->GetTraceHelper()->GetObjectName());
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

