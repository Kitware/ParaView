/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLabel.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLabel.h"

#include "vtkArrayMap.txx"
#include "vtkKWLabel.h"
#include "vtkObjectFactory.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLabel);
vtkCxxRevisionMacro(vtkPVLabel, "1.10");

//----------------------------------------------------------------------------
vtkPVLabel::vtkPVLabel()
{
  this->Label = vtkKWLabel::New();
  this->Label->SetParent(this);
}

//----------------------------------------------------------------------------
vtkPVLabel::~vtkPVLabel()
{
  this->Label->Delete();
  this->Label = NULL;
}

//----------------------------------------------------------------------------
void vtkPVLabel::SetBalloonHelpString(const char *str)
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
    this->Label->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVLabel::Create(vtkKWApplication *pvApp)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }
  
  // Now a label
  this->Label->Create(pvApp, "-width 18 -justify right");
  if (this->BalloonHelpString)
    {
    this->SetBalloonHelpString(this->BalloonHelpString);
    }
  this->Script("pack %s -side left", this->Label->GetWidgetName());  
}


//----------------------------------------------------------------------------
vtkPVLabel* vtkPVLabel::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVLabel::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVLabel::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLabel* pvl = vtkPVLabel::SafeDownCast(clone);
  if (pvl)
    {
    pvl->Label->SetLabel(this->Label->GetLabel());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLabel.");
    }
}

//----------------------------------------------------------------------------
int vtkPVLabel::ReadXMLAttributes(vtkPVXMLElement* element,
                                  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }
  
  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(label)
    {
    this->Label->SetLabel(label);
    }
  else
    {
    this->Label->SetLabel(this->VariableName);
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVLabel::SetLabel(const char *str)
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

//----------------------------------------------------------------------------
const char* vtkPVLabel::GetLabel()
{ 
  return this->Label->GetLabel();
}

//----------------------------------------------------------------------------
void vtkPVLabel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
