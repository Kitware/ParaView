/*=========================================================================

  Module:    vtkKWMenuButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWMenuButton.h"

#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro( vtkKWMenuButton );
vtkCxxRevisionMacro(vtkKWMenuButton, "1.24.2.1");

//----------------------------------------------------------------------------
vtkKWMenuButton::vtkKWMenuButton()
{
  this->CurrentValue      = NULL;
  this->Menu              = vtkKWMenu::New();
}

//----------------------------------------------------------------------------
vtkKWMenuButton::~vtkKWMenuButton()
{
  this->SetCurrentValue(NULL);

  if (this->Menu)
    {
    this->Menu->Delete();
    this->Menu = NULL;
    }
}

//----------------------------------------------------------------------------
const char *vtkKWMenuButton::GetValue()
{
  if (this->IsCreated())
    {
    // Why we we re-assign to CurrentValue each time GetValue() is 
    // called
    // That's because the value of the internal variable is set by Tk
    // through the -variable settings of the radiobutton entries that
    // have been added to the menu. Therefore, if a radiobutton entry has
    // a command that will use the value (very likely), there is no
    // guarantee the variable has been changed before or after calling the
    // callback. To ensure it is true, always refresh the value from
    // the variable itself.
    this->SetCurrentValue(this->Script("set %sValue", this->GetWidgetName()));
    }
  return this->CurrentValue;  
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetValue(const char *s)
{
  if (this->IsCreated() && s && strcmp(s, this->GetValue()))
    {
    this->Script("set %sValue {%s}", this->GetWidgetName(), s);
    this->SetButtonText(s);

    if (this->Menu && *s)
      {
      int nb_items = this->Menu->GetNumberOfItems();
      for (int i = 0; i < nb_items; i++)
        {
        const char *image = this->Menu->GetItemOption(i, "-image");
        if (image && !strcmp(image, s))
          {
          this->SetConfigurationOption("-image", s);
          break;
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::Create(vtkKWApplication *app)
{ 
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::CreateSpecificTkWidget(app, "menubutton"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Menu->SetParent(this);
  this->Menu->Create(app);  

  this->Script("%s config -menu %s", 
               this->GetWidgetName(), this->Menu->GetWidgetName());

  this->IndicatorOn();
  this->SetReliefToRaised();
  this->SetBorderWidth(2);
  this->SetConfigurationOption("-direction", "flush");

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetButtonText(const char *text)
{
  this->SetTextOption(text);
}

//----------------------------------------------------------------------------
const char* vtkKWMenuButton::GetButtonText()
{
  return this->GetTextOption();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::AddCommand(const char* label, vtkKWObject* Object,
                                 const char* MethodAndArgString,
                                 const char* help)
{
  this->Menu->AddCommand(label, Object, MethodAndArgString, help);
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWMenuButton::GetMenu()
{
  return this->Menu;
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetWidth(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::IndicatorOn()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 1", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::IndicatorOff()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 0", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->GetEnabled());

  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

