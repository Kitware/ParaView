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

#include "vtkKWApplication.h"
#include "vtkKWIcon.h"
#include "vtkKWLabel.h"
#include "vtkKWListBox.h"
#include "vtkObjectFactory.h"

#ifdef _MSC_VER
#pragma warning (push, 1)
#pragma warning (disable: 4702)
#endif

#include <vector>
#include <string>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWEntry );
vtkCxxRevisionMacro(vtkKWEntry, "1.50");

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
  if ( this->Entry && this->Entry != this )
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

  if (this->Entry && this->Entry != this)
    {
    this->Entry->SetEnabled(this->Enabled);
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

  int was_disabled = !this->Enabled;
  if (was_disabled)
    {
    this->SetEnabled(1);
    }

  if (this->IsCreated())
    {
    this->Script("%s delete 0 end", this->Entry->GetWidgetName());
    if (s)
      {
      const char *str = this->ConvertInternalStringToTclString(s);
      this->Script("catch {%s insert 0 {%s}}", 
                   this->Entry->GetWidgetName(), str ? str : "");
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
void vtkKWEntry::Create(vtkKWApplication *app, const char *args)
{
  const char *wname;

  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Entry already created");
    return;
    }

  this->SetApplication(app);

  // create the top level
  wname = this->GetWidgetName();

  if ( this->PullDown )
    {
    this->Script("combobox %s %s",wname, (args?args:""));
    this->Entry = this;
    }
  else
    {
    this->Script("entry %s %s",wname, (args?args:""));
    this->Entry = this;
    }
  const char* entry = this->Entry->GetWidgetName();
  this->Script("%s configure -textvariable %sValue", entry, entry);
  if ( this->Width > 0)
    {
    this->Script("%s configure -width %d", entry, this->Width);
    }
  if ( this->ReadOnly )
    {
    this->Script("%s configure -state disabled", entry);
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
  if ( ro && this->GetWidgetName() )
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

  if (this->Application != NULL)
    {
    this->Script("%s configure -width %d", this->GetWidgetName(), width);
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::BindCommand(vtkKWObject *object, 
                             const char *command)
{
  if (this->IsCreated())
    {
    this->Script("bind %s <Return> {%s %s}",
                 this->GetWidgetName(), object->GetTclName(), command);
    this->Script("bind %s <FocusOut> {%s %s}",
                 this->GetWidgetName(), object->GetTclName(), command);
    this->Script("bind %s <Return> {%s %s}",
                 this->Entry->GetWidgetName(), object->GetTclName(), command);
    this->Script("bind %s <FocusOut> {%s %s}",
                 this->Entry->GetWidgetName(), object->GetTclName(), command);
    }
}

//----------------------------------------------------------------------------
void vtkKWEntry::AddValue(const char* value)
{
  if ( this->GetValueIndex(value) >= 0 )
    {
    return;
    }
  // Add to the combo
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetNumberOfValues()
{
  // Get the number of values in combo
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWEntry::DeleteAllValues()
{
  // Delete all values from combo
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWEntry::DeleteValue(int idx)
{
  if ( idx >= (int)this->GetNumberOfValues() )
    {
    vtkErrorMacro("This entry has only: " 
      << this->GetNumberOfValues()
      << " elements. Index " << idx << " is too high");
    return;
    }
  // Delete from combo
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkKWEntry::GetValueFromIndex(int idx)
{
  if ( idx >= this->GetNumberOfValues() )
    {
    return 0;
    }
  // Get value from index
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetValueIndex(const char* value)
{
  int cc;
  for ( cc = 0; cc < this->GetNumberOfValues(); cc ++ )
    {
    if ( this->GetValueFromIndex(cc) && 
      strcmp(this->GetValueFromIndex(cc), value ) == 0 )
      {
      return int(cc);
      }
    }
  return -1;
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

