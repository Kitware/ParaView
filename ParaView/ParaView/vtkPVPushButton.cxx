/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPushButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPushButton.h"

#include "vtkArrayMap.txx"
#include "vtkKWPushButton.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkString.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVPushButton);
vtkCxxRevisionMacro(vtkPVPushButton, "1.9");

//----------------------------------------------------------------------------
vtkPVPushButton::vtkPVPushButton()
{
  this->Button = vtkKWPushButton::New();
  this->EntryLabel = 0;
}

//----------------------------------------------------------------------------
vtkPVPushButton::~vtkPVPushButton()
{
  this->Button->Delete();
  this->SetEntryLabel(0);
}

//----------------------------------------------------------------------------
void vtkPVPushButton::SetLabel(const char* label)
{
  this->SetEntryLabel(label);
  this->Button->SetLabel(label);
}

//----------------------------------------------------------------------------
void vtkPVPushButton::SetBalloonHelpString(const char *str)
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
      this->BalloonHelpString = vtkString::Duplicate(str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->Button->SetBalloonHelpString(this->BalloonHelpString);
    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVPushButton::ExecuteCommand()
{
  this->ModifiedCallback();
  this->AcceptedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPushButton::Create(vtkKWApplication *pvApp)
{
  if (this->Application)
    {
    vtkErrorMacro("PVPushButton already created");
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
  this->Button->SetParent(this);
  this->Button->Create(pvApp, "");
  this->Button->SetLabel(this->EntryLabel);
  this->Button->SetCommand(this, "ExecuteCommand");
  this->Script("pack %s -side left", this->Button->GetWidgetName());
}

//----------------------------------------------------------------------------
vtkPVPushButton* vtkPVPushButton::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVPushButton::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVPushButton::CopyProperties(vtkPVWidget* clone, vtkPVSource* pvSource,
                              vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVPushButton* pvs = vtkPVPushButton::SafeDownCast(clone);
  if (pvs)
    {
    pvs->SetLabel(this->EntryLabel);
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVScale.");
    }
}

//----------------------------------------------------------------------------
int vtkPVPushButton::ReadXMLAttributes(vtkPVXMLElement* element,
                                  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  // Setup the Label.
  const char* label = element->GetAttribute("label");
  if(!label)
    {
    vtkErrorMacro("No label attribute.");
    return 0;
    }
  this->SetLabel(label);

  return 1;
  
}

//-------------------------------------------------------------------------
void vtkPVPushButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "EntryLabel: " << (this->EntryLabel?this->EntryLabel:"none") << endl;
}
