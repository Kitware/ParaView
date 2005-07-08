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

#include "vtkKWApplication.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"
#include "vtkKWIcon.h"
#include "vtkKWTkUtilities.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMenuButton );
vtkCxxRevisionMacro(vtkKWMenuButton, "1.26");

//----------------------------------------------------------------------------
vtkKWMenuButton::vtkKWMenuButton()
{
  this->CurrentValue      = NULL;
  this->Menu              = vtkKWMenu::New();
  this->MaximumLabelWidth = 0;
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
void vtkKWMenuButton::TracedVariableChangedCallback(
  const char *, const char *, const char *)
{
  this->UpdateOptionMenuLabel();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::UpdateOptionMenuLabel()
{
  if (this->IsCreated())
    {
    const char *wname = this->GetWidgetName();
    if (this->MaximumLabelWidth <= 0)
      {
      this->Script("%s configure -text {%s}", wname, this->GetValue());
      }
    else
      {
      vtksys_stl::string cropped = 
        vtksys::SystemTools::CropString(
          this->GetValue(), (size_t)this->MaximumLabelWidth);
      this->Script("%s configure -text {%s}", wname, cropped.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetMaximumLabelWidth(int arg)
{ 
  if (this->MaximumLabelWidth == arg)
    {
    return;
    }

  this->MaximumLabelWidth = arg;
  this->Modified();

  this->UpdateOptionMenuLabel();
}
 
//----------------------------------------------------------------------------
void vtkKWMenuButton::AddRadioButton(const char *label)
{
  this->AddRadioButton(label, 0, 0, 0);
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::AddRadioButton(const char *label, 
                                     vtkObject *obj, 
                                     const char *method,
                                     const char *help)
{
  vtksys_stl::string var(this->GetWidgetName());
  var += "Value";
  this->Menu->AddRadioButton(
    0, label, var.c_str(), obj, method, help);
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::AddRadioButtonImage(const char *image_name, 
                                          vtkObject *obj, 
                                          const char *method,
                                          const char *help)
{
  vtksys_stl::string var(this->GetWidgetName());
  var += "Value";
  this->Menu->AddRadioButtonImage(
    0, image_name, var.c_str(), obj, method, help);
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
  this->Menu->SetTearOff(0);

  const char *wname = this->GetWidgetName();
  
  this->IndicatorOn();
  this->SetReliefToRaised();
  this->SetBorderWidth(2);
  this->SetHighlightThickness(0);
  this->SetAnchorToCenter();

  this->SetConfigurationOption("-direction", "flush");

  this->Script("%s configure -menu %s ",
               wname, this->Menu->GetWidgetName());

  this->Script("set %sValue {}", this->GetWidgetName());
  this->Script("trace variable %sValue w {%s TracedVariableChangedCallback}",
               this->GetWidgetName(), this->GetTclName());

  // Update enable state

  this->UpdateEnableState();
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
void vtkKWMenuButton::SetWidth(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetAnchor(int anchor)
{
  this->SetConfigurationOption(
    "-anchor", vtkKWTkOptions::GetAnchorAsTkOptionValue(anchor));
}

//----------------------------------------------------------------------------
int vtkKWMenuButton::GetAnchor()
{
  return vtkKWTkOptions::GetAnchorFromTkOptionValue(
    this->GetConfigurationOption("-anchor"));
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetImageToPredefinedIcon(int icon_index)
{
  vtkKWIcon *icon = vtkKWIcon::New();
  icon->SetImage(icon_index);
  this->SetImageToIcon(icon);
  icon->Delete();
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetImageToIcon(vtkKWIcon* icon)
{
  if (icon)
    {
    this->SetImageToPixels(
      icon->GetData(), 
      icon->GetWidth(), icon->GetHeight(), icon->GetPixelSize());
    }
}

//----------------------------------------------------------------------------
void vtkKWMenuButton::SetImageToPixels(const unsigned char* pixels, 
                                       int width, 
                                       int height,
                                       int pixel_size,
                                       unsigned long buffer_length)
{
  vtkKWTkUtilities::SetImageOptionToPixels(
    this, pixels, width, height, pixel_size, buffer_length);
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
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Menu: " << this->Menu << endl;
  os << indent << "MaximumLabelWidth: " << this->MaximumLabelWidth << endl;
}

