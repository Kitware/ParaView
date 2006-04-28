/*=========================================================================

  Module:    vtkKWComboBox.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWComboBox.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"

#include "Utilities/BWidgets/vtkKWBWidgetsInit.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWComboBox);
vtkCxxRevisionMacro(vtkKWComboBox, "1.15");

//----------------------------------------------------------------------------
vtkKWComboBox::vtkKWComboBox()
{
  this->Width = 20;
}

//----------------------------------------------------------------------------
void vtkKWComboBox::CreateWidget()
{
  // Use BWidget's ComboBox class:
  // http://aspn.activestate.com/ASPN/docs/ActiveTcl/bwidget/contents.html
  
  vtkKWApplication *app = this->GetApplication();
  vtkKWBWidgetsInit::Initialize(app ? app->GetMainInterp() : NULL);

  // Call the superclass to set the appropriate flags then create manually

  if (!vtkKWWidget::CreateSpecificTkWidget(this, 
        "ComboBox", "-highlightthickness 0"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Configure();
}

//----------------------------------------------------------------------------
void vtkKWComboBox::SetValue(const char *s)
{
  if (!this->IsAlive())
    {
    return;
    }

  int old_state = this->GetState();
  this->SetStateToNormal();

  this->SetTextOption("-text", s);

  this->SetState(old_state);
}

//----------------------------------------------------------------------------
void vtkKWComboBox::AddValue(const char* value)
{
  if (!this->IsCreated() || this->HasValue(value))
    {
    return;
    }

  this->Script("%s configure -values [concat [%s cget -values] {\"%s\"}]", 
    this->GetWidgetName(), this->GetWidgetName(), value);
}

//----------------------------------------------------------------------------
void vtkKWComboBox::AddValueAsInt(int value)
{
  char buffer[256];
  sprintf(buffer, "%d", value);
  this->AddValue(buffer);
}

//----------------------------------------------------------------------------
void vtkKWComboBox::ReplaceNthValue( int idx, const char *value )
{
  if (!this->IsCreated() || this->HasValue(value))
    {
    return;
    }
  if (idx < 0 || idx >= this->GetNumberOfValues())
    {
    vtkErrorMacro(
      "This combobox has only " << this->GetNumberOfValues()
      << " elements. Index " << idx << " is out of range");
    return;
    }

  this->Script("%s configure -values [lreplace [%s cget -values] %d %d %s]", 
    this->GetWidgetName(), this->GetWidgetName(), idx, idx, value);

}

//----------------------------------------------------------------------------
int vtkKWComboBox::GetNumberOfValues()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  return atoi(
    this->Script("llength [%s cget -values]", this->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWComboBox::DeleteAllValues()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s configure -values {}", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWComboBox::DeleteValue(int idx)
{
  if (!this->IsCreated())
    {
    return;
    }
  if (idx < 0 || idx >= this->GetNumberOfValues())
    {
    vtkErrorMacro(
      "This combobox has only " << this->GetNumberOfValues()
      << " elements. Index " << idx << " is out of range");
    return;
    }

  this->Script("%s configure -values [lreplace [%s cget -values] %d %d]", 
    this->GetWidgetName(), this->GetWidgetName(), idx, idx);
}

//----------------------------------------------------------------------------
const char* vtkKWComboBox::GetValueFromIndex(int idx)
{
  if (!this->IsCreated())
    {
    return NULL;
    }
  if (idx < 0 || idx >= this->GetNumberOfValues())
    {
    vtkErrorMacro(
      "This combobox has only " << this->GetNumberOfValues()
      << " elements. Index " << idx << " is out of range");
    return NULL;
    }

  return this->Script("lindex [%s cget -values] %d",
                      this->GetWidgetName(), idx);
}

//----------------------------------------------------------------------------
int vtkKWComboBox::HasValue(const char* value)
{
  return this->GetValueIndex(value) < 0 ? 0 : 1;
}

//----------------------------------------------------------------------------
int vtkKWComboBox::GetValueIndex(const char* value)
{
  if (!this->IsCreated() || !value)
    {
    return -1;
    }
  return atoi(this->Script("lsearch [%s cget -values] {%s}",
                           this->GetWidgetName(), value));
}

//----------------------------------------------------------------------------
void vtkKWComboBox::SetCommand(vtkObject *object, const char *method)
{
  this->Superclass::SetCommand(object, method);

  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, this, "ValueCallback");
    this->SetConfigurationOption("-command", command);
    this->SetConfigurationOption("-modifycmd", command);
    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWComboBox::UpdateEnableState()
{
  this->vtkKWCoreWidget::UpdateEnableState();

  this->SetState(this->GetEnabled());

  if (this->IsCreated())
    {
    this->SetConfigurationOptionAsInt("-editable", this->ReadOnly ? 0 : 1);
    }
}

//----------------------------------------------------------------------------
void vtkKWComboBox::SetListboxWidth(int n)
{
  if (this->IsCreated())
    {
    this->SetConfigurationOptionAsInt("-listboxwidth", n);
    }
}

//----------------------------------------------------------------------------
void vtkKWComboBox::SetBackgroundColor(double r, double g, double b)
{
  this->Superclass::SetBackgroundColor(r, g, b);
  this->SetConfigurationOptionAsColor("-entrybg", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWComboBox::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

