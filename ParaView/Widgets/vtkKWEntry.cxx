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
//============================================================================
class vtkKWEntryInternals
{
public:
  vtkKWEntryInternals() 
    {
    this->Dirty = 1;
    }
  ~vtkKWEntryInternals() {}

  typedef vtkstd::vector<vtkstd::string> VectorOfStrings;
  VectorOfStrings Entries;
  int Dirty;
};
//============================================================================
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWEntry );
vtkCxxRevisionMacro(vtkKWEntry, "1.43");

//----------------------------------------------------------------------------
vtkKWEntry::vtkKWEntry()
{
  this->ValueString = NULL;
  this->Width       = -1;
  this->ReadOnly    = 0;
  this->PullDown    = 0;
  this->Entry       = 0;
  this->TopLevel    = 0;
  this->PopupDisplayed = 0;
  this->List        = 0;

  this->Internals = new vtkKWEntryInternals;
}

//----------------------------------------------------------------------------
vtkKWEntry::~vtkKWEntry()
{
  this->SetValueString(NULL);
  if ( this->Entry && this->Entry != this )
    {
    this->Entry->Delete();
    }
  if ( this->TopLevel )
    {
    this->TopLevel->Delete();
    }
  if ( this->List )
    {
    this->List->Delete();
    }

  delete this->Internals;
}

//----------------------------------------------------------------------------
char *vtkKWEntry::GetValue()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }

  const char *val = this->Script("%s get", this->Entry->GetWidgetName());
  this->SetValueString(this->ConvertTclStringToInternalString(val));
  return this->GetValueString();
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetValueAsInt()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }
  return atoi(this->GetValue());
}

//----------------------------------------------------------------------------
float vtkKWEntry::GetValueAsFloat()
{
  if ( !this->IsCreated() )
    {
    return 0;
    }
  return atof(this->GetValue());
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

  if (this->TopLevel)
    {
    this->TopLevel->SetEnabled(this->Enabled);
    }

  if (this->List)
    {
    this->List->SetEnabled(this->Enabled);
    }
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
  char tmp[1024];
  sprintf(tmp, "%d", i);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(float f)
{
  char tmp[1024];
  sprintf(tmp, "%g", f);
  this->SetValue(tmp);
}

//----------------------------------------------------------------------------
void vtkKWEntry::SetValue(float f, int size)
{
  char tmp[1024];
  char format[1024];
  sprintf(format,"%%.%df",size);
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
    this->Script("frame %s -bd 0", wname);
    this->Entry = vtkKWWidget::New();
    this->Entry->SetParent(this);
    this->Entry->Create(app, "entry", (args?args:""));
    this->Script("pack %s -fill both -expand 1 -side left", this->Entry->GetWidgetName());
    vtkKWLabel *label = vtkKWLabel::New();
    label->SetParent(this);
    label->Create(app, "-relief raised");
    label->SetImageOption(vtkKWIcon::ICON_EXPAND);
    this->Script("pack %s -fill y -expand 0 -side left", label->GetWidgetName());
    label->SetBind(this, "<ButtonPress>", "DisplayPopupCallback");
    label->Delete();
    this->TopLevel = vtkKWWidget::New();
    this->TopLevel->Create(app, "toplevel", "-bg black -bd 1 -relief flat");
    //this->TopLevel->SetBind(this, "<Leave>", "WithdrawPopupCallback");
    this->Script("wm transient %s %s", 
                 this->TopLevel->GetWidgetName(), this->GetWidgetName());
    this->Script("wm overrideredirect %s 1", 
                 this->TopLevel->GetWidgetName());
    this->Script("wm withdraw %s", 
                 this->TopLevel->GetWidgetName());
    this->List = vtkKWListBox::New();
    this->List->SetParent(this->TopLevel);
    this->List->Create(app, 0);
    this->List->SetSingleClickCallback(this, "ValueSelectedCallback");
    this->Script("pack %s -fill both -expand 1", this->List->GetWidgetName());
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
void vtkKWEntry::DisplayPopupCallback()
{
  if (!this->Entry || !this->PullDown || !this->TopLevel )
    {
    return;
    }
  if ( this->PopupDisplayed )
    {
    this->WithdrawPopupCallback();
    return;
    }

  // Get the position of the mouse, the position and size of the push button,
  // the size of the scale.

  this->Script("concat "
               "[winfo rootx %s] [winfo rooty %s] [winfo width %s] [winfo height %s]",
               this->GetWidgetName(), 
               this->GetWidgetName(), 
               this->GetWidgetName(),
               this->GetWidgetName());
  
  int x, y, w, h;
  sscanf(this->Application->GetMainInterp()->result, 
         "%d %d %d %d", 
         &x, &y, &w, &h);

  y = y+h;

  vtkKWEntryInternals::VectorOfStrings::size_type height_mtp
    = this->Internals->Entries.size();
  if ( height_mtp > 5 )
    {
    height_mtp = 5;
    }

  this->Script("wm geometry %s +%d+%d",
               this->TopLevel->GetWidgetName(), x, y);

  if ( this->Internals->Dirty )
    {
    this->List->DeleteAll();
    vtkKWEntryInternals::VectorOfStrings::size_type cc;
    for ( cc = 0; cc < this->Internals->Entries.size(); cc ++ )
      {
      this->List->AppendUnique(this->Internals->Entries[cc].c_str());
      }
    this->Internals->Dirty = 1;
    }
  this->Script("%s configure -width %d", this->List->GetWidgetName(), w);
  this->Script("%s configure -height %d", this->List->GetWidgetName(), height_mtp);
  
  this->Script("update");
  this->Script("wm deiconify %s", 
               this->TopLevel->GetWidgetName());
  this->Script("raise %s", 
               this->TopLevel->GetWidgetName());
  this->Script("%s configure -width %d -height %d", this->TopLevel->GetWidgetName(),
      w, h*height_mtp+5);
  this->PopupDisplayed = 1;
  this->Script("after 5000 { %s WithdrawPopupCallback }", this->GetTclName());
}

// ---------------------------------------------------------------------------
void vtkKWEntry::WithdrawPopupCallback()
{
  if (!this->Entry || !this->PullDown || !this->TopLevel )
    {
    return;
    }
  this->Script("wm withdraw %s",
               this->TopLevel->GetWidgetName());
  this->PopupDisplayed = 0;
  this->Script("after cancel { %s WithdrawPopupCallback }", this->GetTclName());
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
void vtkKWEntry::ValueSelectedCallback()
{
  this->SetValue(this->List->GetSelection());
  this->WithdrawPopupCallback();
  this->Script("event generate %s <Return>", this->Entry->GetWidgetName());
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
  this->Internals->Entries.push_back(value);
  this->Internals->Dirty = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetNumberOfValues()
{
  return static_cast<int>(this->Internals->Entries.size());
}

//----------------------------------------------------------------------------
void vtkKWEntry::DeleteAllValues()
{
  this->Internals->Entries.empty();
  this->Internals->Dirty = 1;
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkKWEntry::DeleteValue(int idx)
{
  if ( idx < (int)this->Internals->Entries.size() )
    {
    vtkErrorMacro("This entry has only: " 
      << static_cast<int>(this->Internals->Entries.size()) 
      << " elements. Index " << idx << " is too high");
    return;
    }
  vtkKWEntryInternals::VectorOfStrings::iterator it;
  int cc = 0;
  for ( it = this->Internals->Entries.begin(); 
        it != this->Internals->Entries.end() && cc < idx; it ++ )
    {
    cc ++;
    }
  if ( cc == idx )
    {
    this->Internals->Entries.erase(it);
    this->Internals->Dirty = 1;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
const char* vtkKWEntry::GetValueFromIndex(int idx)
{
  if ( idx >= this->GetNumberOfValues() )
    {
    return 0;
    }
  return this->Internals->Entries[idx].c_str();
}

//----------------------------------------------------------------------------
int vtkKWEntry::GetValueIndex(const char* value)
{
  vtkKWEntryInternals::VectorOfStrings::size_type cc;
  for ( cc = 0; cc < this->Internals->Entries.size(); cc ++ )
    {
    if ( this->Internals->Entries[cc] == value )
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

