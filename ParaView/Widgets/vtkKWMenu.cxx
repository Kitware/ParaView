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
#include "vtkKWMenu.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"
#include "vtkArrayMap.txx"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMenu );
vtkCxxRevisionMacro(vtkKWMenu, "1.44");



//----------------------------------------------------------------------------
int vtkKWMenuCommand(ClientData cd, Tcl_Interp *interp,
                       int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWMenu::vtkKWMenu()
{
  this->CommandFunction = vtkKWMenuCommand;
  this->TearOff = 0;
}

//----------------------------------------------------------------------------
vtkKWMenu::~vtkKWMenu()
{
}

//----------------------------------------------------------------------------
void vtkKWMenu::Create(vtkKWApplication* app, const char* args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("Menu already created");
    return;
    }
  this->SetApplication(app);

  this->Script("menu %s -tearoff %d %s", 
               this->GetWidgetName(), this->TearOff, (args ? args : "")); 

  this->Script("bind %s <<MenuSelect>> {%s DisplayHelp %%W}", 
               this->GetWidgetName(), this->GetTclName());

  // Update enable state
  
  this->UpdateEnableState();
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetTearOff(int val)
{
  if (val == this->TearOff)
    {
    return;
    }
  this->Modified();
  this->TearOff = val;

  if (this->Application)
    {
    this->Script("%s configure -tearoff %d", this->GetWidgetName(), val);
    }
}


//----------------------------------------------------------------------------
void vtkKWMenu::DisplayHelp(const char* widget)
{
  const char* tname = this->GetTclName();
  this->Script(
    "if [catch {set %sTemp $%sHelpArray([%s entrycget active -label])} %sTemp ]"
    " { set %sTemp \"\"}; set %sTemp", 
    tname, tname, widget, tname, tname, tname );
  if(this->GetApplication()->GetMainInterp()->result)
    {
    vtkKWWindow* window = this->GetWindow();
    if ( window )
      {
      window->SetStatusText(
        this->GetApplication()->GetMainInterp()->result);
      }
    }
}


//----------------------------------------------------------------------------
void vtkKWMenu::AddGeneric(const char* addtype, 
                           const char* label,
                           vtkKWObject* Object,
                           const char* MethodAndArgString,
                           const char* extra, 
                           const char* help)
{
  ostrstream str;
  str << this->GetWidgetName() << " add " << addtype;

  if (label)
    {
    str << " -label {" << label << "}";
    }

  if (Object && MethodAndArgString)
    {
    str << " -command {" << Object->GetTclName() 
        << " " << MethodAndArgString << "}" ;
    }

  if(extra)
    {
    str << " " << extra;
    }

  str << ends;
  
  this->Application->SimpleScript(str.str());
  delete [] str.str();

  if(!help)
    {
    help = label;
    }

  this->Script("set {%sHelpArray(%s)} {%s}", this->GetTclName(), 
               label, help);
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertGeneric(int position, const char* addtype, 
                              const char* label, 
                              vtkKWObject* Object,
                              const char* MethodAndArgString, 
                              const char* extra, 
                              const char* help)
{
  ostrstream str;
  str << this->GetWidgetName() << " insert " << position << " " << addtype;

  if (label)
    {
    str << " -label {" << label << "}";
    }

  if (Object && MethodAndArgString)
    {
    str << " -command {" << Object->GetTclName() 
        << " " << MethodAndArgString << "}" ;
    }

  if(extra)
    {
    str << " " << extra;
    }

  str << ends;
  
  this->Application->SimpleScript(str.str());
  delete [] str.str();

  if(!help)
    {
    help = label;
    }

  this->Script("set {%sHelpArray(%s)} {%s}", this->GetTclName(), 
               label, help);
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddCascade(const char* label, 
                           vtkKWMenu* menu, 
                           int underline, 
                           const char* help)
{
  ostrstream str;
  str << this->GetWidgetName() << " add cascade -label {" << label << "}"
      << " -underline " << underline << ends;
  this->Application->SimpleScript(str.str());
  delete [] str.str();

  if(!help)
    {
    help = label;
    }
  this->Script("set {%sHelpArray(%s)} {%s}", 
               this->GetTclName(), label, help);

  this->SetCascade(label, menu);
}

//----------------------------------------------------------------------------
void  vtkKWMenu::InsertCascade(int position, 
                               const char* label, 
                               vtkKWMenu* menu, 
                               int underline, 
                               const char* help)
{
  ostrstream str;
  
  str << this->GetWidgetName() << " insert " << position 
      << " cascade -label {" << label << "} -underline " << underline << ends;
  this->Application->SimpleScript(str.str());
  delete [] str.str();

  if(!help)
    {
    help = label;
    }
  this->Script("set {%sHelpArray(%s)} {%s}", 
               this->GetTclName(), label, help);

  this->SetCascade(label, menu);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetCascade(int index, const char* menu)
{
  if (!menu)
    {
    return;
    }

  const char *wname = this->GetWidgetName();

  ostrstream str;
  str << wname << " entryconfigure " << index;

  // The cascade menu has to be a child 
  // (i.e. the parent + '.' + at least a letter)
  // If not, clone it.

  int parent_length = (int)(strlen(wname));
  int child_length = (int)(strlen(menu));

  if (child_length < (parent_length + 2) || 
      strncmp(wname, menu, parent_length) ||
      menu[parent_length] != '.')
    {
    ostrstream clone_menu;
    clone_menu << wname << ".clone_";
    this->Script("string trim [%s entrycget %d -label]",  wname, index);
    const char *res = this->GetApplication()->GetMainInterp()->result;
    if (res && *res)
      {
      clone_menu << res;
      }
    else
      {
      clone_menu << index;
      }
    clone_menu << ends;
    this->Script("catch { destroy %s } \n %s clone %s", 
                 clone_menu.str(), menu, clone_menu.str());
    str << " -menu {" << clone_menu.str() << "}" << ends;
    clone_menu.rdbuf()->freeze(0); 
    }
  else
    {
    str << " -menu {" << menu << "}" << ends;
    }

  this->Script(str.str());
  str.rdbuf()->freeze(0); 
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetCascade(int index, vtkKWMenu* menu)
{
  if (!menu)
    {
    return;
    }
  this->SetCascade(index, menu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetCascade(const char* item, vtkKWMenu* menu)
{
  if (!menu )
    {
    return;
    }
  this->SetCascade(this->GetIndex(item), menu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetCascade(const char* item, const char* menu)
{
  if (!menu )
    {
    return;
    }
  this->SetCascade(this->GetIndex(item), menu);
}

//----------------------------------------------------------------------------
void  vtkKWMenu::AddCheckButton(const char* label, const char* ButtonVar, 
                                vtkKWObject* Object, 
                                const char* MethodAndArgString, 
                                const char* help )
{ 
  this->AddCheckButton(label, ButtonVar, Object, MethodAndArgString, -1, help);
}
 
//----------------------------------------------------------------------------
void  vtkKWMenu::AddCheckButton(const char* label, const char* ButtonVar, 
                                vtkKWObject* Object, 
                                const char* MethodAndArgString, 
                                int underline, const char* help )
{ 
  ostrstream str;
  str << "-variable " << ButtonVar;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->AddGeneric("checkbutton", label, Object, 
                   MethodAndArgString, str.str(), help);
  delete [] str.str();
}


//----------------------------------------------------------------------------
void vtkKWMenu::InsertCheckButton(int position, 
                                  const char* label, const char* ButtonVar, 
                                  vtkKWObject* Object, 
                                  const char* MethodAndArgString, const char* help )
{ 
  this->InsertCheckButton( position, label, ButtonVar, Object, MethodAndArgString,
                           -1, help );
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertCheckButton(int position, 
                                  const char* label, const char* ButtonVar, 
                                  vtkKWObject* Object, 
                                  const char* MethodAndArgString, 
                                  int underline, const char* help )
{ 
  ostrstream str;
  str << "-variable " << ButtonVar;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->InsertGeneric(position, "checkbutton", label, Object, 
                      MethodAndArgString, str.str(), help);
  delete [] str.str();
}


//----------------------------------------------------------------------------
void  vtkKWMenu::AddCommand(const char* label, vtkKWObject* Object,
                            const char* MethodAndArgString,
                            int underline, 
                            const char* help)
{
  ostrstream str;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->AddGeneric("command", label, Object, 
                   MethodAndArgString, str.str(), help);
  delete [] str.str();
}

//----------------------------------------------------------------------------
void  vtkKWMenu::AddCommand(const char* label, vtkKWObject* Object,
                            const char* MethodAndArgString ,
                            const char* help)
{
  this->AddGeneric("command", label, Object, 
                   MethodAndArgString, NULL, help);
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertCommand(int position, const char* label, vtkKWObject* Object,
                              const char* MethodAndArgString,
                              int underline, 
                              const char* help)
{
  ostrstream str;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->InsertGeneric(position, "command", label, Object,
                      MethodAndArgString, str.str(), help);
  delete [] str.str();
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertCommand(int position, const char* label, vtkKWObject* Object,
                              const char* MethodAndArgString,
                              const char* help)
{
  this->InsertGeneric(position, "command", label, Object,
                      MethodAndArgString, NULL, help);
}

//----------------------------------------------------------------------------
char* vtkKWMenu::CreateRadioButtonVariable(vtkKWObject* Object, 
                                           const char* varname)
{
  ostrstream str;
  str << Object->GetTclName() << varname << ends;
  return str.str();
}

  
  
//----------------------------------------------------------------------------
int vtkKWMenu::GetRadioButtonValue(vtkKWObject* Object, 
                                   const char* varname)
{
  int res;
  
  char *rbv = 
    this->CreateRadioButtonVariable(Object,varname);
  this->Script("set %s",rbv);
  res = this->GetIntegerResult(this->Application);
  delete [] rbv;
  return res;
}
    
//----------------------------------------------------------------------------
int vtkKWMenu::GetCheckedRadioButtonItem(vtkKWObject* Object, 
                                         const char* varname)
{
  char *rbv = this->CreateRadioButtonVariable(Object,varname);
  int value = this->GetCheckButtonValue(Object,varname);

  int numEntries = this->GetNumberOfItems();
  for(int i = 0; i < numEntries; i++)
    {
    this->Script("%s type %d", this->GetWidgetName(), i);
    if (!strcmp("radiobutton",
                this->GetApplication()->GetMainInterp()->result))
      {
      this->Script("%s entrycget %i -variable", this->GetWidgetName(), i);
      if (!strcmp(rbv, this->GetApplication()->GetMainInterp()->result))
        {
        this->Script("%s entrycget %i -value", this->GetWidgetName(), i);
        if (this->GetIntegerResult(this->Application) == value)
          {
          delete [] rbv;
          return i;
          }
        }
      }
    }

  delete [] rbv;
  return -1;
}
    
//----------------------------------------------------------------------------
void vtkKWMenu::CheckRadioButton(vtkKWObject* Object, 
                                 const char* varname, int id)
{
  char *rbv = 
    this->CreateRadioButtonVariable(Object,varname);
  this->Script("set %s",rbv);
  if (this->GetIntegerResult(this->Application) != id)
    {
    this->Script("set %s %d",rbv,id);
    }
  delete [] rbv;
}

//----------------------------------------------------------------------------
char* vtkKWMenu::CreateCheckButtonVariable(vtkKWObject* Object, 
                                           const char* varname)
{
  ostrstream str;
  str << Object->GetTclName() << varname << ends;
  return str.str();
}

  
  
//----------------------------------------------------------------------------
int vtkKWMenu::GetCheckButtonValue(vtkKWObject* Object, 
                                   const char* varname)
{
  int res;
  
  char *rbv = 
    this->CreateCheckButtonVariable(Object,varname);
  this->Script("set %s",rbv);
  res = this->GetIntegerResult(this->Application);
  delete [] rbv;
  return res;
}
    
//----------------------------------------------------------------------------
void vtkKWMenu::CheckCheckButton(vtkKWObject* Object, 
                                 const char* varname, int id)
{
  char *rbv = 
    this->CreateCheckButtonVariable(Object,varname);
  this->Script("set %s",rbv);
  if (this->GetIntegerResult(this->Application) != id)
    {
    this->Script("set %s %d",rbv,id);
    }
  delete [] rbv;
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddRadioButton(int value, 
                               const char* label, 
                               const char* buttonVar, 
                               vtkKWObject* Object, 
                               const char* MethodAndArgString,
                               int underline, 
                               const char* help)
{
  ostrstream str;
  str << "-value " << value << " -variable " << buttonVar;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->AddGeneric("radiobutton", label, Object,
                   MethodAndArgString, str.str(), help);
  delete [] str.str();
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddRadioButton(int value, const char* label, const char* buttonVar, 
                               vtkKWObject* Object, 
                               const char* MethodAndArgString,
                               const char* help)
{
  this->AddRadioButton(value, label, buttonVar, Object, MethodAndArgString,
                       -1, help);
}


//----------------------------------------------------------------------------
void vtkKWMenu::AddRadioButtonImage(int value, 
                                    const char* imgname, 
                                    const char* buttonVar, 
                                    vtkKWObject* Object, 
                                    const char* MethodAndArgString,
                                    const char* help)
{
  ostrstream str;
  str << "-image " << imgname 
      << " -value " << value 
      << " -variable " << buttonVar
      << ends;
  // Uses the imgname as label, so that the help string can work.
  this->AddGeneric("radiobutton", imgname, Object,
                   MethodAndArgString, str.str(), help);
  delete [] str.str();
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertRadioButton(int position, int value, const char* label, 
                                  const char* buttonVar, 
                                  vtkKWObject* Object, 
                                  const char* MethodAndArgString,
                                  const char* help)
{
  this->InsertRadioButton( position, value, label, buttonVar, Object,
                           MethodAndArgString, -1, help );
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertRadioButton(int position, int value, const char* label, 
                                  const char* buttonVar, 
                                  vtkKWObject* Object, 
                                  const char* MethodAndArgString,
                                  int underline,
                                  const char* help)
{
  ostrstream str;
  str << "-value " << value << " -variable " << buttonVar;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->InsertGeneric(position, "radiobutton", label, Object,
                      MethodAndArgString, str.str(), help);
  delete [] str.str();
}

//----------------------------------------------------------------------------
void vtkKWMenu::Invoke(int position)
{
  this->Script("%s invoke %d", this->GetWidgetName(), position);
}

//----------------------------------------------------------------------------
void vtkKWMenu::Invoke(const char* item)
{
  if ( !this->HasItem(item) )
    {
    return;
    }
  this->Invoke(this->GetIndex(item));
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteMenuItem(int position)
{
  this->Script("catch {%s delete %d}", this->GetWidgetName(), position);
  this->Script("set {%sHelpArray([%s entrycget %d -label])} {}", 
               this->GetWidgetName(), this->GetWidgetName(), 
               position);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteMenuItem(const char* menuitem)
{
  this->Script("catch {%s delete {%s}}", this->GetWidgetName(), menuitem);
  this->Script("set {%sHelpArray(%s)} {}", this->GetWidgetName(), menuitem);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteAllMenuItems()
{
  if ( !this->IsCreated() )
    {
    return;
    }

  int i, last;
  
  this->Script("%s index end", this->GetWidgetName());
  if (strcmp("none", this->GetApplication()->GetMainInterp()->result) == 0)
    {
    return;
    }
  
  last = vtkKWObject::GetIntegerResult(this->Application);
  
  for (i = last; i >= 0; --i)
    {
    this->DeleteMenuItem(i);
    }
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndex(const char* menuname)
{
  this->Script("%s index {%s}", this->GetWidgetName(), menuname);
  return vtkKWObject::GetIntegerResult(this->Application);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemLabel(int position, char* label, int maxlen)
{
  if (!this->IsCreated() || !label)
    {
    return VTK_ERROR;
    }
  const char* lbl = 
    this->Script("%s entrycget %d -label", this->GetWidgetName(), position);
  if (!lbl[0]) 
    {
    return VTK_ERROR;
    }
  strncpy(label, lbl, maxlen);
  return VTK_OK;
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemLabel(int position)
{
  if (this->IsCreated())
    {
    if (position >= 0 && position < this->GetNumberOfItems())
      {
      return this->Script("%s entrycget %d -label", 
                          this->GetWidgetName(), position);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWMenu::HasItem(const char* menuname)
{
  this->Script("catch {%s index {%s}}", this->GetWidgetName(), menuname);
  return !vtkKWObject::GetIntegerResult(this->Application);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetNumberOfItems()
{
  if (this->IsCreated())
    {
    const char *end = this->Script("%s index end", this->GetWidgetName());
    if (strcmp(end, "none"))
      {
      return atoi(end) + 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddSeparator()
{
  this->Script( "%s add separator", this->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertSeparator(int position)
{
  this->Script( "%s insert %d separator", this->GetWidgetName(), position);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetState(int index)
{
  const char* state = this->Script("%s entrycget %d -state", 
                                   this->GetWidgetName(), index);
  if (!state || !state[0])
    {
    return vtkKWMenu::Unknown;
    }
  if ( strcmp(state, "normal") == 0 )
    {
    return vtkKWMenu::Normal;
    }
  else if ( strcmp(state, "active") == 0 )
    {
    return vtkKWMenu::Active;
    }
  else if ( strcmp(state, "disabled") == 0 )
    {
    return vtkKWMenu::Disabled;
    }
  return vtkKWMenu::Unknown;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetState(const char* item)
{
  if ( !this->HasItem(item) )
    {
    return vtkKWMenu::Unknown;
    }
  int index = this->GetIndex(item);
  return this->GetState(index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetState(int index, int state)
{
  char stateStr[][9] = { "normal", "active", "disabled" };
  if ( state <= vtkKWMenu::Normal || state > vtkKWMenu::Disabled )
    {
    state = 0;
    }
  this->Script("catch {%s entryconfigure %d -state %s}", 
               this->GetWidgetName(), index, stateStr[state] );
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetState(const char* item, int state)
{
  if ( !this->HasItem(item) )
    {
    return;
    }
  int index = this->GetIndex(item);
  this->SetState(index, state);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetState(int state)
{
  int nb_of_items = this->GetNumberOfItems();
  for (int i = 0; i < nb_of_items; i++)
    {
    this->SetState(i, state);
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::ConfigureItem(int index, const char* conf)
{
  ostrstream str;
  str << this->GetWidgetName() << " entryconfigure "
      << index << " " << conf << ends;
  this->Script(str.str());
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(int index, vtkKWObject* object, 
                           const char* MethodAndArgString)
{
  ostrstream str;
  str << this->GetWidgetName() << " entryconfigure "
      << index << " -command {" << object->GetTclName() 
      << " " << MethodAndArgString << "}" << ends;
  this->Script(str.str());
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(int idx, const char* MethodAndArgString)
{
  if ( !this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems() )
    {
    return;
    }
  ostrstream str;
  str << this->GetWidgetName() << " entryconfigure "
      << idx << " -command {" << MethodAndArgString << "}" << ends;
  this->Script(str.str());
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(const char* item, const char* MethodAndArgString)
{
  if ( !this->HasItem(item) )
    {
    return;
    }
  int index = this->GetIndex(item);
  ostrstream str;
  str << this->GetWidgetName() << " entryconfigure "
      << index << " -command {" << MethodAndArgString << "}" << ends;
  this->Script(str.str());
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(const char* item, vtkKWObject* object, 
                           const char* MethodAndArgString)
{
  if ( !this->HasItem(item) )
    {
    return;
    }
  int index = this->GetIndex(item);
  this->SetEntryCommand(index, object, MethodAndArgString);
}


//----------------------------------------------------------------------------
void vtkKWMenu::StoreMenuState(vtkArrayMap<const char*, int>* state)
{
  state->RemoveAllItems();
  int numEntries = this->GetNumberOfItems();
  for(int i = 0; i < numEntries; i++)
    {
    char label[128];
    if (this->GetItemLabel(i, label, 128) == VTK_OK)
      {
      state->SetItem(label, this->GetState(i));
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::RestoreMenuState(vtkArrayMap<const char*, int>* state)
{
  vtkArrayMapIterator<const char*, int>* it = state->NewIterator();

  // Mark all sources as not visited.
  while( !it->IsDoneWithTraversal() )
    {    
    int state = 0;
    const char* item = 0;
    if (it->GetKey(item) == VTK_OK && item && it->GetData(state) == VTK_OK)
      {
      if ( state == vtkKWMenu::Active )
        {
        this->SetState(item, vtkKWMenu::Normal);
        }
      else
        {
        this->SetState(item, state);
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemOption(int idx, const char *option)
{
  if ( !this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems() )
    {
    return 0;
    }
  return this->Script("%s entrycget %d %s", 
    this->GetWidgetName(), idx, option);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemOption(const char *item, const char *option)
{
  return this->GetItemOption(this->GetIndex(item), option);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemCommand(int idx)
{
  return this->GetItemOption(idx, "-command");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCompoundImage(int idx, const char *imagename)
{
#if (TK_MAJOR_VERSION == 8) && (TK_MINOR_VERSION >= 4)
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -compound center -image %s -hidemargin 0", 
               this->GetWidgetName(), idx, imagename);
#endif
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCompoundImage(const char *item, const char *imagename)
{
  this->SetItemCompoundImage(this->GetIndex(item), imagename);
}

//----------------------------------------------------------------------------
void vtkKWMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->Enabled ? vtkKWMenu::Normal : vtkKWMenu::Disabled);
}

//----------------------------------------------------------------------------
void vtkKWMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TearOff: " << this->GetTearOff() << endl;
}

