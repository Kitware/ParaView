/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLabeledToggle.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLabeledToggle.h"

#include "vtkArrayMap.txx"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMSourceProxy.h"
#include "vtkPVTraceHelper.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLabeledToggle);
vtkCxxRevisionMacro(vtkPVLabeledToggle, "1.44");

//----------------------------------------------------------------------------
vtkPVLabeledToggle::vtkPVLabeledToggle()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->CheckButton = vtkKWCheckButton::New();
  this->CheckButton->SetParent(this);
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle::~vtkPVLabeledToggle()
{
  this->CheckButton->Delete();
  this->CheckButton = NULL;
  this->Label->Delete();
  this->Label = NULL;
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetBalloonHelpString(const char *str)
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->Label)
    {
    this->Label->SetBalloonHelpString(str);
    }

  if (this->CheckButton)
    {
    this->CheckButton->SetBalloonHelpString(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();
  
  // Now a label
  this->Label->Create();
  this->Label->SetWidth(18);
  this->Label->SetJustificationToRight();
  this->Script("pack %s -side left", this->Label->GetWidgetName());
  
  // Now the check button
  this->CheckButton->Create();
  this->CheckButton->SetCommand(this, "CheckButtonCallback");
  this->Script("pack %s -side left", this->CheckButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::CheckButtonCallback(int)
{
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetSelectedState(int val)
{
  int oldVal;
  
  oldVal = this->CheckButton->GetSelectedState();
  if (val == oldVal)
    {
    return;
    }

  this->CheckButton->SetSelectedState(val);
  
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
int vtkPVLabeledToggle::GetSelectedState() 
{ 
  return this->CheckButton->GetSelectedState(); 
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Disable()
{
  this->Script("%s configure -state disabled", 
               this->CheckButton->GetWidgetName());
  // TCL 8.2 does not allow to disable a label. Use the checkbutton's
  // color to make it look like disabled.
  this->Script("%s configure -foreground [%s cget -disabledforeground]", 
               this->Label->GetWidgetName(),
               this->CheckButton->GetWidgetName());
}

//---------------------------------------------------------------------------
void vtkPVLabeledToggle::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetSelectedState "
        << this->GetSelectedState() << endl;
}

//-----------------------------------------------------------------------------
void vtkPVLabeledToggle::SaveInBatchScript(ofstream *file)
{
  const char* sourceID = this->PVSource->GetProxy()->GetSelfIDAsString();
  
  if (!sourceID || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  *file << "  [$pvTemp" << sourceID << " GetProperty "
        << this->SMPropertyName << "] SetElement 0 "
        << this->GetSelectedState() << endl;
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Accept()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  
  if (ivp)
    {
    ivp->SetElement(0, this->GetSelectedState());
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

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::Initialize()
{
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());
  if (ivp)
    {
    this->SetSelectedState(ivp->GetElement(0));
    }
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::ResetInternal()
{
  this->Initialize();
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVLabeledToggle* vtkPVLabeledToggle::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVLabeledToggle::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::CopyProperties(vtkPVWidget* clone, 
                                        vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLabeledToggle* pvlt = vtkPVLabeledToggle::SafeDownCast(clone);
  if (pvlt)
    {
    const char* label = this->Label->GetText();
    pvlt->Label->SetText(label);

    if (label && label[0] &&
        (pvlt->GetTraceHelper()->GetObjectNameState() == 
         vtkPVTraceHelper::ObjectNameStateUninitialized ||
         pvlt->GetTraceHelper()->GetObjectNameState() == 
         vtkPVTraceHelper::ObjectNameStateDefault) )
      {
      pvlt->GetTraceHelper()->SetObjectName(label);
      pvlt->GetTraceHelper()->SetObjectNameState(
        vtkPVTraceHelper::ObjectNameStateSelfInitialized);
      }
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLabeledToggle.");
    }
}

//----------------------------------------------------------------------------
int vtkPVLabeledToggle::ReadXMLAttributes(vtkPVXMLElement* element,
                                          vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->Label->SetText(label);
    }
  else
    {
    this->Label->SetText(this->GetTraceHelper()->GetObjectName());
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::SetLabel(const char *str) 
{
  this->Label->SetText(str); 
  if (str && str[0] &&
      (this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateUninitialized ||
       this->GetTraceHelper()->GetObjectNameState() == 
       vtkPVTraceHelper::ObjectNameStateDefault) )
    {
    this->GetTraceHelper()->SetObjectName(str);
    this->GetTraceHelper()->SetObjectNameState(
      vtkPVTraceHelper::ObjectNameStateSelfInitialized);
    }
}

//----------------------------------------------------------------------------
const char* vtkPVLabeledToggle::GetLabel() 
{ 
  return this->Label->GetText();
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->CheckButton);
}

//----------------------------------------------------------------------------
void vtkPVLabeledToggle::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
