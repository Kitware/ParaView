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
#include "vtkKWEvent.h"
#include "vtkKWLabel.h"
#include "vtkKWMenu.h"
#include "vtkKWScale.h"
#include "vtkObjectFactory.h"
#include "vtkPVAnimationInterfaceEntry.h"
#include "vtkPVApplication.h"
#include "vtkPVProcessModule.h"
#include "vtkPVScalarListWidgetProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkClientServerStream.h"

#include <vtkstd/string>

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVScale);
vtkCxxRevisionMacro(vtkPVScale, "1.37");

//----------------------------------------------------------------------------
vtkPVScale::vtkPVScale()
{
  this->EntryLabel = 0;
  this->LabelWidget = vtkKWLabel::New();
  this->Scale = vtkKWScale::New();
  this->Round = 0;
  this->RangeSourceVariable = 0;
  this->Property = 0;
  this->DefaultValue = 0.0;
  this->AcceptedValueInitialized = 0;
  this->EntryFlag = 0;
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
  this->SetProperty(NULL);
  this->SetRangeSourceVariable(0);
}

void vtkPVScale::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->LabelWidget->SetLabel(label);
}

void vtkPVScale::SetBalloonHelpString(const char *str)
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
    this->Scale->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
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
  this->InvokeEvent(vtkKWEvent::WidgetModifiedEvent, 0);
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
  if (this->Application)
    {
    vtkErrorMacro("PVScale already created");
    return;
    }

  // For getting the widget in a script.
  if (this->EntryLabel && this->EntryLabel[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(this->EntryLabel);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  
  this->SetApplication(pvApp);

  // create the top level
  this->Script("frame %s -borderwidth 0 -relief flat", this->GetWidgetName());

  // Now a label
  this->LabelWidget->SetParent(this);
  this->LabelWidget->Create(pvApp, "-width 18 -justify right");
  this->LabelWidget->SetLabel(this->EntryLabel);
  this->Script("pack %s -side left", this->LabelWidget->GetWidgetName());

  this->Scale->SetParent(this);
  if (this->DisplayValueFlag)
    {
    this->Scale->Create(this->Application, "-showvalue 1");
    }
  else
    {
    this->Scale->Create(this->Application, "-showvalue 0");
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
  
  this->SetBalloonHelpString(this->BalloonHelpString);
  this->Script("pack %s -side left -fill x -expand t", 
               this->Scale->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVScale::SetValue(float val)
{
  float newVal;
  float oldVal;
  
  if(this->Round)
    {
    newVal = this->RoundValue(val);
    }
  else
    {
    newVal = val;
    }
  
  if (this->Property && !this->AcceptedValueInitialized)
    {
    this->Property->SetScalars(1, &newVal);
    this->AcceptedValueInitialized = 1;
    }
  
  oldVal = this->Scale->GetValue();
  if (newVal == oldVal)
    {
    this->Scale->SetValue(newVal); // to keep the entry in sync with the scale
    return;
    }

  this->Scale->SetValue(newVal);
  
  this->ModifiedCallback();
}


//-----------------------------------------------------------------------------
void vtkPVScale::SaveInBatchScript(ofstream *file)
{
  float scalar, scaleValue = this->GetValue();
  if(this->Round)
    {
    scalar = this->RoundValue(scaleValue);
    }
  else
    {
    scalar = scaleValue;
    }

  *file << "  [$pvTemp" << this->PVSource->GetVTKSourceID(0) 
        <<  " GetProperty " << this->VariableName << "] SetElements1 "
        << scalar << endl;
}


//----------------------------------------------------------------------------
void vtkPVScale::AcceptInternal(vtkClientServerID sourceID)
{
  if (sourceID.ID && this->VariableName)
    { 
    float scalar, scaleValue = this->GetValue();
    if (this->EntryFlag)
      {
      float entryValue;
      entryValue = this->Scale->GetEntry()->GetValueAsFloat();
      if (entryValue != scaleValue)
        {
        scaleValue = entryValue;
        this->Scale->SetValue(entryValue);
        }
      }
    if(this->Round)
      {
      scalar = this->RoundValue(scaleValue);
      }
    else
      {
      scalar = scaleValue;
      }
    this->UpdateVTKSourceInternal(sourceID, scalar);
    }

  this->ModifiedFlag = 0;
}

//---------------------------------------------------------------------------
void vtkPVScale::Trace()
{
  if (this->Application && this->Application->GetTraceFile())
    {
    this->Trace(this->Application->GetTraceFile());
    }
}

//---------------------------------------------------------------------------
void vtkPVScale::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->Scale->GetValue() << endl;
}


//----------------------------------------------------------------------------
void vtkPVScale::ResetInternal()
{
 if (this->Property)
    {
    this->SetValue(this->Property->GetScalar(0));
    }
  if ( this->ObjectID.ID != 0 && this->RangeSourceVariable )
    {
    vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
    ostrstream str;
    str << "Get" << this->RangeSourceVariable << ends;
    pm->GetStream() << vtkClientServerStream::Invoke << this->ObjectID
                    << str.str() << vtkClientServerStream::End;
    pm->SendStreamToServerRoot();
    int range[2] = { 0, 0 };
    pm->GetLastServerResult().GetArgument(0,0, range, 2);
    this->Script("eval %s SetRange %i %i", this->Scale->GetTclName(), 
      range[0], range[1]);
    }

  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
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
    float min, max;
    this->Scale->GetRange(min, max);
    pvs->SetRange(min, max);
    pvs->SetDefaultValue(this->GetDefaultValue());
    pvs->SetResolution(this->Scale->GetResolution());
    pvs->SetLabel(this->EntryLabel);
    pvs->SetRangeSourceVariable(this->RangeSourceVariable);
    pvs->SetEntryFlag(this->EntryFlag);
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
float vtkPVScale::GetValue() 
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
    label = element->GetAttribute("variable");
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

  float range[2];
  if(!element->GetVectorAttribute("range",2,range))
    {
    range[0] = 0;
    range[1] = 100;
    }
  this->SetRange(range[0], range[1]);

  // Setup the default value.
  float default_value;
  if(!element->GetScalarAttribute("default_value",&default_value))
    {
    this->SetDefaultValue(range[0]);
    }
  else
    {
    if (default_value < range[0] )
      {
      this->SetDefaultValue(range[0]);
      }
    else if (default_value > range[1])
      {
      this->SetDefaultValue(range[1]);
      }
    else
      {
      this->SetDefaultValue(default_value);
      }
    }

  const char* range_source = element->GetAttribute("range_source");
  if(range_source)
    {
    this->SetRangeSourceVariable(range_source);
    }

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
  os << indent << "Round: " << this->Round << endl;
  os << indent << "EntryFlag: " << this->EntryFlag << endl;
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
  menu->AddCommand(this->LabelWidget->GetLabel(), this, methodAndArgs, 0,"");
}

//----------------------------------------------------------------------------
void vtkPVScale::SetObjectVariableToPVTime(int time)
{
  this->UpdateVTKSourceInternal(this->ObjectID, time);
}
  
//----------------------------------------------------------------------------
void vtkPVScale::AnimationMenuCallback(vtkPVAnimationInterfaceEntry *ai)
{
  char script[500];
  
  if (ai->InitializeTrace(NULL))
    {
    this->AddTraceEntry("$kw(%s) AnimationMenuCallback $kw(%s)", 
                        this->GetTclName(), ai->GetTclName());
    }
  
  // I do not like setting the label like this but ...
  sprintf(script, "%s SetObjectVariableToPVTime $pvTime", 
          this->GetTclName());
  ai->SetLabelAndScript(this->LabelWidget->GetLabel(), script, this->GetTraceName());
  ai->SetCurrentProperty(this->Property);
  ai->SetTimeStart(this->GetRangeMin());
  ai->SetTimeEnd(this->GetRangeMax());
  ai->SetTypeToFloat();
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
void vtkPVScale::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVScalarListWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    this->Property->SetScalars(1, &this->DefaultValue);
    }
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVScale::GetProperty()
{
  return this->Property;
}

//----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVScale::CreateAppropriateProperty()
{
  return vtkPVScalarListWidgetProperty::New();
}

//----------------------------------------------------------------------------
void vtkPVScale::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LabelWidget);
  this->PropagateEnableState(this->Scale);
}

//----------------------------------------------------------------------------
void vtkPVScale::UpdateVTKSourceInternal(vtkClientServerID sourceID,
                                         float value)
{
  vtkPVProcessModule* pm = this->GetPVApplication()->GetProcessModule();
  vtkstd::string method = "Set";
  method += this->VariableName;
  pm->GetStream() << vtkClientServerStream::Invoke
                  << sourceID << method.c_str() << value
                  << vtkClientServerStream::End;
  pm->SendStreamToServer();
  this->Property->SetScalars(1, &value);
}
