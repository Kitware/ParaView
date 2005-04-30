/*=========================================================================

  Module:    vtkKWOptionMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWOptionMenu.h"

#include "vtkKWApplication.h"
#include "vtkKWMenu.h"
#include "vtkObjectFactory.h"

#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWOptionMenu );
vtkCxxRevisionMacro(vtkKWOptionMenu, "1.35");

//----------------------------------------------------------------------------
vtkKWOptionMenu::vtkKWOptionMenu()
{
  this->CurrentValue      = NULL;
  this->Menu              = vtkKWMenu::New();
  this->MaximumLabelWidth = 0;
}

//----------------------------------------------------------------------------
vtkKWOptionMenu::~vtkKWOptionMenu()
{
  this->SetCurrentValue(NULL);

  if (this->Menu)
    {
    this->Menu->Delete();
    this->Menu = NULL;
    }
}

//----------------------------------------------------------------------------
const char *vtkKWOptionMenu::GetValue()
{
  if (this->IsCreated())
    {
    // Why we we re-assign to CurrentValue each time GetValue() is called
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
void vtkKWOptionMenu::SetValue(const char *s)
{
  if (this->IsCreated() && s)
    {
    this->Script("set %sValue {%s}", this->GetWidgetName(), s);
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::SetCurrentEntry(const char *name)
{ 
  this->SetValue(name);
}
 
//----------------------------------------------------------------------------
void vtkKWOptionMenu::TracedVariableChangedCallback(
  const char *, const char *, const char *)
{
  this->UpdateOptionMenuLabel();
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::UpdateOptionMenuLabel()
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
      kwsys_stl::string cropped = 
        kwsys::SystemTools::CropString(
          this->GetValue(), (size_t)this->MaximumLabelWidth);
      this->Script("%s configure -text {%s}", wname, cropped.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::SetMaximumLabelWidth(int arg)
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
void vtkKWOptionMenu::SetCurrentImageEntry(const char *image_name)
{ 
  if (this->IsCreated())
    {
    this->Script("%s configure -image %s", this->GetWidgetName(), image_name);
    this->SetValue(image_name);
    }
}
 
//----------------------------------------------------------------------------
const char* vtkKWOptionMenu::GetEntryLabel(int index)
{ 
  return this->Menu->GetItemLabel(index);
}

//----------------------------------------------------------------------------
int vtkKWOptionMenu::GetNumberOfEntries()
{ 
  return this->Menu->GetNumberOfItems();
}
 
//----------------------------------------------------------------------------
void vtkKWOptionMenu::AddEntry(const char *name)
{
  this->AddEntryWithCommand(name, 0, 0, 0);
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::AddEntryWithCommand(const char *name, 
                                          vtkKWObject *obj, 
                                          const char *method,
                                          const char *options)
{
  ostrstream extra;
  extra << "-variable " << this->GetWidgetName() << "Value";
  if (options)
    {
    extra << " " << options;
    }
  extra << ends;
  this->Menu->AddGeneric("radiobutton", name, obj, method, extra.str(), 0);
  extra.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::AddImageEntryWithCommand(const char *image_name, 
                                               vtkKWObject *obj, 
                                               const char *method,
                                               const char *options)
{
  ostrstream extra;
  if (image_name)
    {
    extra << "-image " << image_name << " -selectimage " << image_name;
    }
  if (options)
    {
    extra << " " << options;
    }
  extra << ends;
  this->AddEntryWithCommand(image_name, obj, method, extra.str());
  extra.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::AddSeparator()
{
  this->Menu->AddSeparator();
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::DeleteEntry(const char* name)
{ 
  this->Menu->DeleteMenuItem(name);
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::DeleteEntry(int index)
{
  this->Menu->DeleteMenuItem(index);
}

//----------------------------------------------------------------------------
int vtkKWOptionMenu::HasEntry(const char *name)
{
  return this->Menu->HasItem(name);
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::DeleteAllEntries()
{
  this->Menu->DeleteAllMenuItems();
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "menubutton", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Menu->SetParent(this);
  this->Menu->Create(app, "-tearoff 0");

  const char *wname = this->GetWidgetName();
  
  this->Script("%s configure -indicatoron 1 -menu %s "
               "-relief raised -bd 2 -highlightthickness 0 -anchor c "
               "-direction flush %s", 
               wname, this->Menu->GetWidgetName(), (args ? args : ""));

  this->Script("set %sValue {}", this->GetWidgetName());
  this->Script("trace variable %sValue w {%s TracedVariableChangedCallback}",
               this->GetWidgetName(), this->GetTclName());

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::IndicatorOn()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 1", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::IndicatorOff()
{
  if (!this->IsCreated())
    {
    return;
    }

  this->Script("%s config -indicatoron 0", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::SetWidth(int width)
{
  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetStateOption(this->GetEnabled());
  this->PropagateEnableState(this->Menu);
}

//----------------------------------------------------------------------------
void vtkKWOptionMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Menu: " << this->Menu << endl;
  os << indent << "MaximumLabelWidth: " << this->MaximumLabelWidth << endl;
}

