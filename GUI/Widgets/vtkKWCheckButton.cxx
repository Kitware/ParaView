/*=========================================================================

  Module:    vtkKWCheckButton.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWCheckButton );
vtkCxxRevisionMacro(vtkKWCheckButton, "1.32");

//----------------------------------------------------------------------------
vtkKWCheckButton::vtkKWCheckButton() 
{
  this->IndicatorOn = 1;
  this->MyText = 0;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkKWCheckButton::~vtkKWCheckButton() 
{
  this->SetMyText(0);
  this->SetVariableName(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetVariableName(const char* _arg)
{
  if (this->VariableName == NULL && _arg == NULL) 
    { 
    return;
    }

  if (this->VariableName && _arg && (!strcmp(this->VariableName, _arg))) 
    { 
    return;
    }

  int has_old_state = 0, old_state = 0;
  if (this->VariableName) 
    { 
    if (_arg)
      {
      has_old_state = 1;
      old_state = this->GetState();
      }
    delete [] this->VariableName; 
    }

  if (_arg)
    {
    this->VariableName = new char[strlen(_arg)+1];
    strcpy(this->VariableName,_arg);
    }
   else
    {
    this->VariableName = NULL;
    }

  this->Modified();
  
  if (this->IsCreated() && this->VariableName)
    {
    this->Script("%s configure -variable {%s}", 
                 this->GetWidgetName(), this->VariableName);
    if (has_old_state)
      {
      this->SetState(old_state);
      }
    }
} 

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetIndicator(int ind)
{
  if (ind != this->IndicatorOn)
    {
    this->IndicatorOn = ind;
    this->Modified();
    if (this->IsCreated())
      {
      this->Script("%s configure -indicatoron %d", 
                   this->GetWidgetName(), (ind ? 1 : 0));
      }
    }
  this->SetMyText(0);
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetText(const char* txt)
{
  this->SetMyText(txt);
  this->SetTextOption(txt);
}

//----------------------------------------------------------------------------
const char* vtkKWCheckButton::GetText()
{
  return this->MyText;
}

//----------------------------------------------------------------------------
int vtkKWCheckButton::GetState()
{
  if (this->IsCreated())
    {
    return atoi(
      this->Script("expr {${%s}} == {[%s cget -onvalue]}",
                   this->VariableName, this->GetWidgetName()));
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetState(int s)
{
  if (this->IsCreated())
    {
    int was_disabled = !this->GetEnabled();
    if (was_disabled)
      {
      this->SetEnabled(1);
      }

    if (s)
      {
      this->Script("%s select",this->GetWidgetName());
      }
    else
      {
      this->Script("%s deselect",this->GetWidgetName());
      }

    if (was_disabled)
      {
      this->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "checkbutton", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Configure();
  this->ConfigureOptions(args);

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::Configure()
{
  const char *wname = this->GetWidgetName();

  this->Script("%s configure -indicatoron %d",
               wname, (this->IndicatorOn ? 1 : 0));

  this->SetTextOption(this->MyText);

  // Set the variable name if not set already
  if (!this->VariableName)
    {
    char *vname = new char [strlen(wname) + 5 + 1];
    sprintf(vname, "%sValue", wname);
    this->SetVariableName(vname);
    delete [] vname;
    }
  else
    {
    this->Script("%s configure -variable {%s}", wname, this->VariableName);
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(int icon_index,
                                      const char *blend_color_option,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(
    icon_index, blend_color_option, image_option);
  if (!blend_color_option && !image_option)
    {
    this->Superclass::SetImageOption(
      icon_index, "-selectcolor", "-selectimage");
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(vtkKWIcon* icon,
                                      const char *blend_color_option,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(icon, blend_color_option, image_option);
  if (!blend_color_option && !image_option)
    {
    this->Superclass::SetImageOption(icon, "-selectcolor", "-selectimage");
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(const unsigned char* data, 
                                      int width, 
                                      int height,
                                      int pixel_size,
                                      unsigned long buffer_length,
                                      const char *blend_color_option,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(
    data, width, height, pixel_size, buffer_length, 
    blend_color_option, image_option);
  if (!blend_color_option && !image_option)
    {
    this->Superclass::SetImageOption(
      data, width, height, pixel_size, buffer_length, 
      "-selectcolor", "-selectimage");
    }
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::SetImageOption(const char *image_name,
                                      const char *image_option)
{
  this->Superclass::SetImageOption(image_name, image_option);
}

// ---------------------------------------------------------------------------
void vtkKWCheckButton::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWCheckButton::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "VariableName: " 
     << (this->VariableName ? this->VariableName : "None" );
}

