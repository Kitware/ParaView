/*=========================================================================

  Module:    vtkKWEntry.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWEntry.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWEntry);
vtkCxxRevisionMacro(vtkKWEntry, "1.64");

//----------------------------------------------------------------------------
vtkKWEntry::vtkKWEntry()
{
  this->ValueString = NULL;
  this->Width       = -1;
  this->ReadOnly    = 0;
  this->PullDown    = 0;
  this->Entry       = 0;
}

//----------------------------------------------------------------------------
vtkKWEntry::~vtkKWEntry()
{
  this->SetValueString(NULL);
  if ( this->Entry && this->Entry != this)
    {
    this->Entry->Delete();
    }
}

//----------------------------------------------------------------------------
char *vtkKWEntry::GetValue()
{
  if (!this->IsCreated())
    {
    return NULL;
    }

  const char *val = this->Script("%s get", this->Entry->GetWidgetName());
  this->SetValueString(this->ConvertTclStringToInternalString(val));
  return this->GetValueString();
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetValueAsInt()
{
  if (!this->IsCreated())
    {
    return 0;
    }
  
  char *val = this->GetValue();
  if (!val || !*val)
    {
    return 0;
    }

  return atoi(val);
}

//----------------------------------------------------------------------------
double vtkKWEntry::GetValueAsFloat()
{
  if (!this->IsCreated())
    {
    return 0;
    }

  char *val = this->GetValue();
  if (!val || !*val)
    {
    return 0;
    }

  return atof(val);
}

//----------------------------------------------------------------------------
void vtkKWEntry::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Entry)
    {
    this->SetStateOption(this->GetEnabled());
    if (this->Entry != this)
      {
      this->PropagateEnableState(this->Entry);
      }
    }

#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION < 4)
  if (this->Entry && this->Entry->IsCreated())
    {
    if (this->Enabled)
      {
      this->Script("%s configure -foreground black -background white", 
                   this->Entry->GetWidgetName());
      }
    else
      {
      this->Script("%s configure -foreground gray70 -background gray90", 
                   this->Entry->GetWidgetName());
      }
    }
#endif
}


//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(const char *s)
{
  int ro = 0;
  if (this->ReadOnly)
    {
    this->ReadOnlyOff();
    ro = 1;
    }

  int was_disabled = !this->GetEnabled();
  if (was_disabled)
    {
    this->SetEnabled(1);
    }

  if (this->Entry && this->Entry->IsAlive())
    {
    this->Script("%s delete 0 end", this->Entry->GetWidgetName());
    if (s)
      {
      const char *val = this->ConvertInternalStringToTclString(
        s, vtkKWCoreWidget::ConvertStringEscapeInterpretable);
      this->Script("%s insert 0 \"%s\"", 
                   this->Entry->GetWidgetName(), val ? val : "");
      }
    }

  if (was_disabled)
    {
    this->SetEnabled(0);
    }

  if (ro)
    {
    this->ReadOnlyOn();
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(int i)
{
  char *val = this->GetValue();
  if (val && *val && i == this->GetValueAsInt())
    {
    return;
    }

  char tmp[1024];
  sprintf(tmp, "%d", i);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(double f)
{
  char *val = this->GetValue();
  if (val && *val && f == this->GetValueAsFloat())
    {
    return;
    }

  char tmp[1024];
  sprintf(tmp, "%.5g", f);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(double f, int size)
{
  char *val = this->GetValue();
  if (val && *val && f == this->GetValueAsFloat())
    {
    return;
    }

  char tmp[1024];
  char format[1024];
  sprintf(format,"%%.%dg",size);
  sprintf(tmp,format, f);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::Create(vtkKWApplication *app)
{
  // Call the superclass to set the appropriate flags then create manually

  if (!this->Superclass::CreateSpecificTkWidget(
        app, this->PullDown ? "ComboBox" : "entry"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->Entry = this;

  const char* entry_wname = this->Entry->GetWidgetName();
  this->Script("%s configure -textvariable %sValue", entry_wname, entry_wname);
  if (this->Width > 0)
    {
    this->Script("%s configure -width %d", entry_wname, this->Width);
    }
  if (this->ReadOnly)
    {
    this->Script("%s configure -state disabled", entry_wname);
    }

  // Update enable state

  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetReadOnly(int ro)
{
  this->ReadOnly = ro;
  if (!this->IsCreated())
    {
    return;
    }
  if ( ro && this->GetWidgetName())
    {
    this->Script("%s configure -state disabled", this->Entry->GetWidgetName());
    }
  else
    {
    this->Script("%s configure -state normal", this->Entry->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetWidth(int width)
{
  if (this->Width == width)
    {
    return;
    }

  this->Modified();
  this->Width = width;

  if (this->IsCreated())
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::BindCommand(vtkObject *object, 
                             const char *method)
{
  if (this->IsCreated())
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);

    this->Script("bind %s <Return> {%s}",
                 this->GetWidgetName(), command);
    this->Script("bind %s <FocusOut> {%s}",
                 this->GetWidgetName(), command);
    this->Script("bind %s <Return> {%s}",
                 this->Entry->GetWidgetName(), command);
    this->Script("bind %s <FocusOut> {%s}",
                 this->Entry->GetWidgetName(), command);

    delete [] command;
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::AddValue(const char* value)
{
  if (!this->Entry || !this->Entry->IsCreated() || !this->PullDown || 
      this->GetValueIndex(value) >= 0)
    {
    return;
    }

  this->Script("%s configure -values [concat [%s cget -values] {%s}]", 
    this->Entry->GetWidgetName(), this->Entry->GetWidgetName(), value);
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetNumberOfValues()
{
  if (!this->Entry || !this->Entry->IsCreated() || !this->PullDown)
    {
    return 0;
    }

  return atoi(this->Script("llength [%s cget -values]",
                           this->Entry->GetWidgetName()));
}

//----------------------------------------------------------------------------
void vtkKWEntry::DeleteAllValues()
{
  if (!this->Entry || !this->Entry->IsCreated() || !this->PullDown)
    {
    return;
    }

  this->Script("%s configure -values {}", this->Entry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWEntry::DeleteValue(int idx)
{
  if (!this->Entry || !this->Entry->IsCreated() || !this->PullDown)
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
    this->Entry->GetWidgetName(), this->Entry->GetWidgetName(), idx, idx);
}

//----------------------------------------------------------------------------
const char* vtkKWEntry::GetValueFromIndex(int idx)
{
  if (!this->Entry || !this->Entry->IsCreated() || !this->PullDown)
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
                      this->Entry->GetWidgetName(), idx);
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetValueIndex(const char* value)
{
  if (!this->Entry || !this->Entry->IsCreated() || !this->PullDown || !value)
    {
    return -1;
    }
  return atoi(this->Script("lsearch [%s cget -values] {%s}",
                           this->Entry->GetWidgetName(), value));
}

//----------------------------------------------------------------------------
void vtkKWEntry::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Width: " << this->GetWidth() << endl;
  os << indent << "Readonly: " << (this->ReadOnly?"on":"off") << endl;
  os << indent << "PullDown: " << (this->PullDown?"on":"off") << endl;
  os << indent << "Entry: " << this->Entry << endl;
}

