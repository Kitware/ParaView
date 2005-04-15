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
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkSMStringListDomain.h"
#include "vtkSMStringVectorProperty.h"
#include "vtkPVTraceHelper.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVStringEntry);
vtkCxxRevisionMacro(vtkPVStringEntry, "1.46");

//----------------------------------------------------------------------------
vtkPVStringEntry::vtkPVStringEntry()
{
  this->LabelWidget = vtkKWLabel::New();
  this->LabelWidget->SetParent(this);
  this->Entry = vtkKWEntry::New();
  this->Entry->SetParent(this);
  this->EntryLabel = 0;
}

//----------------------------------------------------------------------------
vtkPVStringEntry::~vtkPVStringEntry()
{
  this->Entry->Delete();
  this->Entry = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
  this->SetEntryLabel(0);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetText(label);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::SetBalloonHelpString(const char *str)
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->LabelWidget)
    {
    this->LabelWidget->SetBalloonHelpString(str);
    }

  if (this->Entry)
    {
    this->Entry->SetBalloonHelpString(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

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
  
  // Now the entry
  this->Entry->Create(pvApp, "");
  this->Script("bind %s <KeyPress> {%s ModifiedCallback}",
               this->Entry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t",
               this->Entry->GetWidgetName());
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

  this->ModifiedCallback();
}
  
//----------------------------------------------------------------------------
void vtkPVStringEntry::Accept()
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (svp)
    {
    svp->SetElement(0, this->GetValue());
    }
  else
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    }

  this->Superclass::Accept();
}

//---------------------------------------------------------------------------
void vtkPVStringEntry::Trace(ofstream *file)
{
  if (this->GetTraceHelper()->Initialize(file))
    {
    *file << "$kw(" << this->GetTclName() << ") SetValue {"
          << this->GetValue() << "}" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::Initialize()
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());

  vtkSMStringListDomain* dom = vtkSMStringListDomain::SafeDownCast(
    svp->GetDomain("default_value"));
  if (dom && dom->GetNumberOfStrings() > 0)
    {
    if (dom->GetNumberOfStrings() > 0)
      {
      this->SetValue(dom->GetString(0));
      }
    }
  else
    {
    this->SetValue(svp->GetElement(0));
    }
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::ResetInternal()
{
  vtkSMStringVectorProperty *svp = vtkSMStringVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (svp)
    {
    this->SetValue(svp->GetElement(0));
    }

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVStringEntry* vtkPVStringEntry::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVStringEntry::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVStringEntry::CopyProperties(vtkPVWidget* clone, 
                                      vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVStringEntry* pvse = vtkPVStringEntry::SafeDownCast(clone);
  if (pvse)
    {
    pvse->SetLabel(this->EntryLabel);
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
    this->SetLabel(this->GetTraceHelper()->GetObjectName());
    }

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
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }

  *file << "  [$pvTemp" << sourceID << " GetProperty " << this->SMPropertyName
        << "] SetElement 0 {" << this->GetValue() << "}" << endl;
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
