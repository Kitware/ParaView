/*=========================================================================

  Program:   ParaView
  Module:    vtkPVThumbWheel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVThumbWheel.h"

#include "vtkClientServerID.h"
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWThumbWheel.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVScalarListWidgetProperty.h"
#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLPackageParser.h"

//-----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVThumbWheel);
vtkCxxRevisionMacro(vtkPVThumbWheel, "1.2");

//-----------------------------------------------------------------------------
vtkPVThumbWheel::vtkPVThumbWheel()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
  this->ThumbWheel = vtkKWThumbWheel::New();
  this->ThumbWheel->SetParent(this);
  this->Property = NULL;
  this->AcceptedValueInitialized = 0;
  this->DefaultValue = 0.0;
}

//-----------------------------------------------------------------------------
vtkPVThumbWheel::~vtkPVThumbWheel()
{
  this->Label->Delete();
  this->ThumbWheel->Delete();
  this->SetProperty(NULL);
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::Create(vtkKWApplication *pvApp)
{
  const char *wname;
  
  if (this->Application)
    {
    vtkErrorMacro("PVThumbWheel already created");
    return;
    }
  
  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);
  
  // Now a label
  this->Label->Create(pvApp, "-justify right");
  if (strlen(this->Label->GetLabel()) > 0)
    {
    this->Label->SetWidth(18);
    }
  this->Script("pack %s -side left", this->Label->GetWidgetName());
  
  // Now the thumb wheel
  this->ThumbWheel->PopupModeOn();
  this->ThumbWheel->Create(pvApp, "");
  this->ThumbWheel->DisplayEntryOn();
  this->ThumbWheel->DisplayEntryAndLabelOnTopOff();
  this->ThumbWheel->ExpandEntryOn();
  this->ThumbWheel->ClampMinimumValueOn();
  this->ThumbWheel->SetInteractionModeToNonLinear(0);
  this->ThumbWheel->SetNonLinearMaximumMultiplier(10);
  this->ThumbWheel->SetEndCommand(this, "ModifiedCallback");
  this->ThumbWheel->GetEntry()->SetBind(this, "<KeyRelease>",
                                        "ModifiedCallback");
  
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left -fill x -expand 1", this->ThumbWheel->GetWidgetName());
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::SetValue(float val)
{
  float oldVal = static_cast<float>(this->ThumbWheel->GetValue());
  if (oldVal == val)
    {
    return;
    }

  if (this->Property && !this->AcceptedValueInitialized)
    {
    this->Property->SetScalars(1, &val);
    this->AcceptedValueInitialized = 1;
    }
  
  this->ThumbWheel->SetValue(val);
  
  this->ModifiedCallback();
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::SetResolution(float res)
{
  this->ThumbWheel->SetResolution(res);
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::SetMinimumValue(float min)
{
  this->ThumbWheel->SetMinimumValue(min);
}

//-----------------------------------------------------------------------------
float vtkPVThumbWheel::GetValue()
{
  return this->ThumbWheel->GetValue();
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::SetLabel(const char *str)
{
  this->Label->SetLabel(str);
  if (str && str[0] &&
      (this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName(str);
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::SetBalloonHelpString(const char *str)
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
    this->Label->SetBalloonHelpString(this->BalloonHelpString);
    this->ThumbWheel->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//-----------------------------------------------------------------------------
vtkPVThumbWheel* vtkPVThumbWheel::ClonePrototype(
  vtkPVSource *pvSource, vtkArrayMap<vtkPVWidget*, vtkPVWidget*>*map)
{
  vtkPVWidget *clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVThumbWheel::SafeDownCast(clone);
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::AcceptInternal(vtkClientServerID sourceID)
{
  if (!sourceID.ID)
    {
    return;
    }
  
  float scalar = this->ThumbWheel->GetValue();
  float entryValue = this->ThumbWheel->GetEntry()->GetValueAsFloat();
  if (entryValue != scalar)
    {
    scalar = entryValue;
    this->ThumbWheel->SetValue(entryValue);
    }
  this->Property->SetScalars(1, &scalar);
  this->Property->SetVTKSourceID(sourceID);
  this->Property->AcceptInternal();
  this->ModifiedFlag = 0;
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::ResetInternal()
{
  if (this->Property)
    {
    this->SetValue(this->Property->GetScalar(0));
    }
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::Trace(ofstream *file)
{
  if (!this->InitializeTrace(file))
    {
    return;
    }
  
  *file << "$kw(" << this->GetTclName() << ") SetValue "
        << this->GetValue() << endl;
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::SetProperty(vtkPVWidgetProperty *prop)
{
  this->Property = vtkPVScalarListWidgetProperty::SafeDownCast(prop);
  if (this->Property)
    {
    this->Property->SetScalars(1, &this->DefaultValue);
    char *cmd = new char[strlen(this->VariableName)+4];
    sprintf(cmd, "Set%s", this->VariableName);
    int numVars = 1;
    this->Property->SetVTKCommands(1, &cmd, &numVars);
    delete [] cmd;
    }
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVThumbWheel::GetProperty()
{
  return this->Property;
}

//-----------------------------------------------------------------------------
vtkPVWidgetProperty* vtkPVThumbWheel::CreateAppropriateProperty()
{
  return vtkPVScalarListWidgetProperty::New();
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();
  
  this->PropagateEnableState(this->Label);
  this->PropagateEnableState(this->ThumbWheel);
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::CopyProperties(vtkPVWidget *clone, vtkPVSource *source,
                                  vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, source, map);
  vtkPVThumbWheel *pvtw = vtkPVThumbWheel::SafeDownCast(clone);
  if (pvtw)
    {
    pvtw->SetMinimumValue(this->ThumbWheel->GetMinimumValue());
    pvtw->SetDefaultValue(this->GetDefaultValue());
    pvtw->SetResolution(this->ThumbWheel->GetResolution());
    pvtw->SetLabel(this->Label->GetLabel());
    }
  else
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVThumbWheel.");
    }
}

//-----------------------------------------------------------------------------
int vtkPVThumbWheel::ReadXMLAttributes(vtkPVXMLElement *element,
                                       vtkPVXMLPackageParser *parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the label
  const char *label = element->GetAttribute("label");
  if (!label)
    {
    label = element->GetAttribute("variable");
    
    if (!label)
      {
      vtkErrorMacro("No label attribute.");
      return 0;
      }
    }
  this->SetLabel(label);
  
  // Setup the resolution
  float resolution;
  if (!element->GetScalarAttribute("resolution", &resolution))
    {
    resolution = 1;
    }
  this->SetResolution(resolution);
 
  // Setup the minimum value
  float min;
  if (!element->GetScalarAttribute("minimum_value", &min))
    {
    min = 0;
    }
  this->SetMinimumValue(min);
  
  // Setup the default value.
  float default_value;
  if (!element->GetScalarAttribute("default_value", &default_value))
    {
    this->SetDefaultValue(min);
    }
  else
    {
    if (default_value < min)
      {
      this->SetDefaultValue(min);
      }
    else
      {
      this->SetDefaultValue(default_value);
      }
    }
  
  return 1;
}

//-----------------------------------------------------------------------------
void vtkPVThumbWheel::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
