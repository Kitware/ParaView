/*=========================================================================

  Program:   ParaView
  Module:    vtkPVStringEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVStringEntry.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVStringWidgetProperty.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkClientServerStream.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVStringEntry);
vtkCxxRevisionMacro(vtkPVStringEntry, "1.32");

//----------------------------------------------------------------------------
vtkPVStringEntry::vtkPVStringEntry()
{
  this->LabelWidget = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->EntryLabel = 0;
  this->DefaultValue = 0;
  this->Property = 0;

  this->InitSourceVariable = 0;
}

//----------------------------------------------------------------------------
vtkPVStringEntry::~vtkPVStringEntry()
{
  this->Entry->Delete();
  this->Entry = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetEntryLabel(0);
  this->SetProperty(0);
  this->SetInitSourceVariable(0);;
  this->SetDefaultValue(0);
}

void vtkPVStringEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

void vtkPVStringEntry::SetBalloonHelpString(const char *str)
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
    this->Entry->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::Create(vtkKWApplication *pvApp)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("StringEntry already created");
    return;
    }
  this->SetApplication(pvApp);

  const char* wname;
  
  // For getting the widget in a script.
  if (this->EntryLabel && this->EntryLabel[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->EntryLabel);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  
  
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
  
  // Now the entry
  this->Entry->Create(pvApp, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->Entry->GetWidgetName(), this->GetTclName());
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());
  this->SetValue(this->Property->GetString());
}


//----------------------------------------------------------------------------
void vtkPVStringEntry::SetValue(const char* fileName)
{
  const char *old;
  
  if (fileName == NULL)
    {
    fileName = "";
    }

  old = this->Entry->GetValue();
  if ( old && fileName )
    {
    if (strcmp(old, fileName) == 0)
      {
      return;
      }
    }

  this->Entry->SetValue(fileName); 

  if (!this->AcceptCalled && this->Property)
    {
    this->Property->SetString(fileName);
    }
  
  this->ModifiedCallback();
}

  
//----------------------------------------------------------------------------
void vtkPVStringEntry::AcceptInternal(vtkClientServerID sourceID)
{
  this->ModifiedFlag = 0;
  
  this->Property->SetString(this->GetValue());
  this->Property->SetVTKSourceID(sourceID);
  this->Property->AcceptInternal();
}

//---------------------------------------------------------------------------
void vtkPVStringEntry::Trace(ofstream *file)
{
  if (this->InitializeTrace(file))
    {
    *file << "$kw(" << this->GetTclName() << ") SetValue {"
          << this->GetValue() << "}" << endl;
    }
}


//----------------------------------------------------------------------------
void vtkPVStringEntry::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  // Command to update the UI.
  this->SetValue(this->Property->GetString());

  if ( this->ObjectID.ID && this->InitSourceVariable )
    {
    vtkstd::string method = "Get";
    method += this->InitSourceVariable;
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke
                    << this->ObjectID << method.c_str()
                    << vtkClientServerStream::End;
    pm->SendStream(vtkProcessModule::DATA_SERVER_ROOT);
    const char* value;
    if(pm->GetLastServerResult().GetArgument(0, 0, &value))
      {
      this->SetValue(value);
      }
    else
      {
      vtkErrorMacro("Error getting \"" << this->InitSourceVariable
                    << "\" value from server.");
      }
    }

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
}

//----------------------------------------------------------------------------
vtkPVStringEntry* vtkPVStringEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVStringEntry::SafeDownCast(clone);
}

void vtkPVStringEntry::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVStringEntry* pvse = vtkPVStringEntry::SafeDownCast(clone);
  if (pvse)
    {
    pvse->SetLabel(this->EntryLabel);
    pvse->SetDefaultValue(this->DefaultValue);
    pvse->SetInitSourceVariable(this->InitSourceVariable);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVStringEntry.");
    }
}

//----------------------------------------------------------------------------
int vtkPVStringEntry::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
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

  const char* init_source = element->GetAttribute("init_source");
  if ( init_source )
    {
    this->SetInitSourceVariable(init_source);
    }
  
  // Set the default value.
  const char* defaultValue = element->GetAttribute("default_value");
  this->SetDefaultValue(defaultValue);
  
  return 1;
}

//----------------------------------------------------------------------------
const char* vtkPVStringEntry::GetValue() 
{
  return this->Entry->GetValue();
}


//-----------------------------------------------------------------------------
void vtkPVStringEntry::SaveInBatchScript(ofstream *file)
{
  const char* str = this->Property->GetString();
  *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
        << " GetProperty " << this->VariableName << "] SetElement 0 {"
        << str << "}" << endl;
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVStringWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    this->Property->SetString(this->DefaultValue);
    char *cmd = new char[strlen(this->VariableName)+4];
    sprintf(cmd, "Set%s", this->VariableName);
    this->Property->SetVTKCommand(cmd);
    delete [] cmd;
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVStringEntry::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVStringEntry::CreateAppropriateProperty()
{
  return vtkPVStringWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabelWidget);
  this->PropagateEnableState(this->Entry);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
