/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
vtkCxxRevisionMacro(vtkKWEntry, "1.37.2.2");

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

