/*=========================================================================

  Program:   ParaView
  Module:    vtkPVScale.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVScale.h"

#include "vtkArrayMap.txx"
#include "vtkKWEntry.h"
#include "vtkCommand.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWPushButton.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkSMDoubleRangeDomain.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntRangeDomain.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkPVTraceHelper.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScale);
vtkCxxRevisionMacro(vtkPVScale, "1.63");

//----------------------------------------------------------------------------
vtkPVScale::vtkPVScale()
{
  this->EntryLabel = 0;
  this->LabelWidget = vtkKWLabel::New();
  this->Scale = vtkKWScale::New();
  this->EntryFlag = 0;
  this->Round = 0;
  this->EntryAndLabelOnTopFlag = 1;
  this->DisplayValueFlag = 1;
  this->TraceSliderMovement = 0;
}

//----------------------------------------------------------------------------
vtkPVScale::~vtkPVScale()
{
  this->SetEntryLabel(0);
  this->Scale->Delete();
  this->Scale = NULL;
  this->LabelWidget->Delete();
  this->LabelWidget = NULL;
}

//----------------------------------------------------------------------------
void vtkPVScale::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetText(label);
}

//----------------------------------------------------------------------------
void vtkPVScale::SetBalloonHelpString(const char *str)
{
  this->Superclass::SetBalloonHelpString(str);

  if (this->LabelWidget)
    {
    this->LabelWidget->SetBalloonHelpString(str);
    }

  if (this->Scale)
    {
    this->Scale->SetBalloonHelpString(str);
    }
}

//----------------------------------------------------------------------------
void vtkPVScale::SetResolution(float res)
{
  this->Scale->SetResolution(res);
}

//----------------------------------------------------------------------------
void vtkPVScale::SetRange(float min, float max)
{
  this->Scale->SetRange(min, max);
}

//----------------------------------------------------------------------------
float vtkPVScale::GetRangeMin()
{
  return this->Scale->GetRangeMin();
}

//----------------------------------------------------------------------------
float vtkPVScale::GetRangeMax()
{
  return this->Scale->GetRangeMax();
}

//----------------------------------------------------------------------------
void vtkPVScale::DisplayEntry()
{
  this->Scale->DisplayEntry();
  this->EntryFlag = 1;
}

//----------------------------------------------------------------------------
void vtkPVScale::SetDisplayEntryAndLabelOnTop(int value)
{
  this->Scale->SetDisplayEntryAndLabelOnTop(value);
  this->EntryAndLabelOnTopFlag = value;
}

//----------------------------------------------------------------------------
void vtkPVScale::CheckModifiedCallback()
{
  this->ModifiedCallback();
  this->AcceptedCallback();
  this->InvokeEvent(vtkCommand::WidgetModifiedEvent, 0);
}

//----------------------------------------------------------------------------
void vtkPVScale::EntryCheckModifiedCallback()
{
  if (!this->EntryFlag)
    {
    return;
    }
  
  this->Scale->SetValue(this->Scale->GetEntry()->GetValueAsFloat());
  this->CheckModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVScale::Create(vtkKWApplication *pvApp)
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
  this->LabelWidget->SetParent(this);
  this->LabelWidget->Create(pvApp, "-width 18 -justify right");
  this->LabelWidget->SetText(this->EntryLabel);
  this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());

  this->Scale->SetParent(this);
  if (this->DisplayValueFlag)
    {
    this->Scale->Create(this->GetApplication(), "-showvalue 1");
    }
  else
    {
    this->Scale->Create(this->GetApplication(), "-showvalue 0");
    }

  this->Scale->SetCommand(this, "CheckModifiedCallback");
  if (this->TraceSliderMovement)
    {
    this->Scale->SetEndCommand(this, "Trace");
    }
  
  if (this->EntryFlag)
    {
    this->DisplayEntry();
    this->Script("bind %s <KeyPress> {%s CheckModifiedCallback}",
                 this->Scale->GetEntry()->GetWidgetName(), this->GetTclName());
    }
  this->SetDisplayEntryAndLabelOnTop(this->EntryAndLabelOnTopFlag);
  
  this->Script("pack %s -side left -fill x -expand t", 
               this->Scale->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVScale::SetValue(double val)
{
  this->SetValueInternal(val);
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVScale::SetValueInternal(double val)
{
  double newVal;

  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if(ivp || this->Round)
    {
    newVal = this->RoundValue(val);
    }
  else
    {
    newVal = val;
    }

/*
  double oldVal;
  oldVal = this->Scale->GetValue();
  if (newVal == oldVal)
    {
    this->Scale->SetValue(newVal); // to keep the entry in sync with the scale
    return;
    }
*/
  int old_disable = this->Scale->GetDisableCommands();
  this->Scale->SetDisableCommands(1);
  this->Scale->SetValue(newVal); 
  this->Scale->SetDisableCommands(old_disable);
}

//-----------------------------------------------------------------------------
void vtkPVScale::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  
  if (sourceID.ID == 0 || !this->SMPropertyName)
    {
    vtkErrorMacro("Sanity check failed. " << this->GetClassName());
    return;
    }
  
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  *file << "  [$pvTemp" << sourceID << " GetProperty "
        << this->SMPropertyName << "] SetElement 0 ";
  if (ivp || this->Round)
    {
    *file << this->RoundValue(this->GetValue()) << endl;
    }
  else if (dvp)
    {
    *file << this->GetValue() << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVScale::Accept()
{
  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if (!dvp && !ivp)
    {
    vtkErrorMacro(
      "Could not find property of name: "
      << (this->GetSMPropertyName()?this->GetSMPropertyName():"(null)")
      << " for widget: " << this->GetTraceHelper()->GetObjectName());
    }

  if (this->EntryFlag)
    {
    double entryValue;
    entryValue = this->Scale->GetEntry()->GetValueAsFloat();
    if (entryValue != this->GetValue())
      {
      this->Scale->SetValue(entryValue);
      }
    }

  if (dvp)
    {
    dvp->SetElement(0, this->GetValue());
    }
  else if (ivp)
    {
    ivp->SetElement(0, this->RoundValue(this->GetValue()));
    }

  this->Superclass::Accept();
}

//---------------------------------------------------------------------------
void vtkPVScale::Trace()
{
  
  vtkPVApplication *pvapp = 
    vtkPVApplication::SafeDownCast(this->GetApplication());
  if (pvapp && pvapp->GetTraceFile())
    {
    this->Trace(pvapp->GetTraceFile());
    }
}

//---------------------------------------------------------------------------
void vtkPVScale::Trace(ofstream *file)
{
  if ( ! this->GetTraceHelper()->Initialize(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->Scale->GetValue() << endl;
}

//----------------------------------------------------------------------------
void vtkPVScale::Initialize()
{
  vtkSMProperty* prop = this->GetSMProperty();

  vtkSMDoubleVectorProperty *dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->GetSMProperty());
  vtkSMIntVectorProperty *ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->GetSMProperty());

  if (prop)
    {
    vtkSMDoubleRangeDomain* drd = vtkSMDoubleRangeDomain::SafeDownCast(
      prop->GetDomain("range"));
    vtkSMIntRangeDomain* ird = vtkSMIntRangeDomain::SafeDownCast(
      prop->GetDomain("range"));
    int minExists = 0, maxExists = 0;
    if (ird)
      {
      int min = ird->GetMinimum(0, minExists);
      int max = ird->GetMaximum(0, maxExists);
      if (minExists && maxExists)
        {
        this->Scale->SetRange(min, max);
        }
      }
    else if (drd)
      {
      double min = drd->GetMinimum(0, minExists);
      double max = drd->GetMaximum(0, maxExists);
      if (minExists && maxExists)
        {
        this->Scale->SetRange(min, max);
        }
      }
    else
      {
      vtkErrorMacro("Could not find a required domain (range) for property "
                    << prop->GetClassName() << ": " << prop->GetXMLName());
      }
    }

  if (dvp)
    {
    this->SetValueInternal(dvp->GetElement(0));
    }
  else if (ivp)
    {
    this->SetValueInternal(ivp->GetElement(0));
    }

}

//----------------------------------------------------------------------------
void vtkPVScale::ResetInternal()
{
  this->Initialize();
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
vtkPVScale* vtkPVScale::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVScale::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVScale::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVScale* pvs = vtkPVScale::SafeDownCast(clone);
  if (pvs)
    {
    //float min, max;
    //this->Scale->GetRange(min, max);
    //pvs->SetRange(min, max);
    pvs->SetResolution(this->Scale->GetResolution());
    pvs->SetLabel(this->EntryLabel);
    pvs->SetEntryFlag(this->EntryFlag);
    pvs->SetRound(this->Round);
    pvs->SetEntryAndLabelOnTopFlag(this->EntryAndLabelOnTopFlag);
    pvs->SetDisplayValueFlag(this->DisplayValueFlag);
    pvs->SetTraceSliderMovement(this->GetTraceSliderMovement());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVScale.");
    }
}

//----------------------------------------------------------------------------
double vtkPVScale::GetValue() 
{ 
  return this->Scale->GetValue(); 
}

//----------------------------------------------------------------------------
int vtkPVScale::ReadXMLAttributes(vtkPVXMLElement* element,
                                  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    label = element->GetAttribute("trace_name");
    if (!label )
      {
      vtkErrorMacro("No label attribute.");
      return 0;
      }
    }
  this->SetLabel(label);

  // Setup the Resolution.
  float resolution;
  if(!element->GetScalarAttribute("resolution",&resolution))
    {
    resolution = 1;
    }
  this->SetResolution(resolution);

  const char* display_entry = element->GetAttribute("display_entry");
  if (display_entry)
    {
    this->EntryFlag = atoi(display_entry);
    }
  
  const char* display_top = element->GetAttribute("entry_and_label_on_top");
  if (display_top)
    {
    this->EntryAndLabelOnTopFlag = atoi(display_top);
    }

  const char* display_value = element->GetAttribute("display_value");
  if (display_value)
    {
    this->DisplayValueFlag = atoi(display_value);
    }
  
  
  const char *slider_movement = element->GetAttribute("trace_slider_movement");
  if (slider_movement)
    {
    this->TraceSliderMovement = atoi(slider_movement);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVScale::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "EntryFlag: " << this->EntryFlag << endl;
  os << indent << "Round: " << this->Round << endl;
  os << indent << "EntryAndLabelOnTopFlag: " << this->EntryAndLabelOnTopFlag
     << endl;
  os << indent << "DisplayValueFlag: " << this->DisplayValueFlag << endl;
  os << indent << "TraceSliderMovement: " << this->TraceSliderMovement << endl;
}

//----------------------------------------------------------------------------
void vtkPVScale::AddAnimationScriptsToMenu(vtkKWMenu *menu, 
                                           vtkPVAnimationInterfaceEntry *ai)
{
  char methodAndArgs[500];
  
  sprintf(methodAndArgs, "AnimationMenuCallback %s", ai->GetTclName()); 
  menu->AddCommand(this->LabelWidget->GetText(), this, methodAndArgs, 0,"");
}

//-----------------------------------------------------------------------------
void vtkPVScale::ResetAnimationRange(vtkPVAnimationInterfaceEntry *ai)
{
  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDomain *dom = prop->GetDomain("range");

  int minSet = 0;
  int maxSet = 0;
  if (dom)
    {
    vtkSMIntRangeDomain *iDom = vtkSMIntRangeDomain::SafeDownCast(dom);
    vtkSMDoubleRangeDomain *dDom = vtkSMDoubleRangeDomain::SafeDownCast(dom);
    int minExists = 0, maxExists = 0;
    if (iDom)
      {
      int min = iDom->GetMinimum(0, minExists);
      int max = iDom->GetMaximum(0, maxExists);
      if (minExists)
        {
        ai->SetTimeStart(min);
        minSet = 1;
        }
      if (maxExists)
        {
        ai->SetTimeEnd(max);
        maxSet = 1;
        }
      }
    else if (dDom)
      {
      double min = dDom->GetMinimum(0, minExists);
      double max = dDom->GetMaximum(0, maxExists);
      if (minExists)
        {
        ai->SetTimeStart(min);
        minSet = 1;
        }
      if (maxExists)
        {
        ai->SetTimeEnd(max);
        maxSet = 1;
        }
      }
    else
      {
      vtkErrorMacro("Could not find required domain (range)");
      }
    }

  if (!minSet)
    {
    ai->SetTimeStart(this->GetRangeMin());
    }
  if (!maxSet)
    {
    ai->SetTimeEnd(this->GetRangeMax());
    }
}

//----------------------------------------------------------------------------
void vtkPVScale::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  if (ai->GetTraceHelper()->Initialize())
    {
    this->GetTraceHelper()->AddEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }
  
  this->Superclass::AnimationMenuCallback(ai);

  ai->SetLabelAndScript(
    this->LabelWidget->GetText(), NULL, this->GetTraceHelper()->GetObjectName());
  
  vtkSMProperty *prop = this->GetSMProperty();
  vtkSMDomain *dom = prop->GetDomain("range");

  char methodAndArgs[500];
  
  sprintf(methodAndArgs, "ResetAnimationRange %s", ai->GetTclName());
  ai->GetResetRangeButton()->SetCommand(this, methodAndArgs);
  ai->SetResetRangeButtonState(1);
  ai->UpdateEnableState();
  
  ai->SetCurrentSMProperty(prop);
  ai->SetCurrentSMDomain(dom);
  ai->SetAnimationElement(0);
  
  this->ResetAnimationRange(ai);

  ai->Update();
}

//----------------------------------------------------------------------------
int vtkPVScale::RoundValue(float val)
{
  if(val >= 0)
    {
    return static_cast<int>(val+0.5);
    }
  else
    {
    return -static_cast<int>((-val)+0.5);
    }
}

//----------------------------------------------------------------------------
void vtkPVScale::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabelWidget);
  this->PropagateEnableState(this->Scale);
}
