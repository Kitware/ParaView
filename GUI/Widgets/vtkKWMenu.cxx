/*=========================================================================

  Module:    vtkKWMenu.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMenu.h"

#include "vtkObjectFactory.h"
#include "vtkKWApplication.h"
#include "vtkKWWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMenu );
vtkCxxRevisionMacro(vtkKWMenu, "1.59");



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
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "menu", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  const char *wname = this->GetWidgetName();
  this->Script("%s configure -tearoff %d", wname, this->TearOff); 

  this->Script("bind %s <<MenuSelect>> {%s DisplayHelp %%W}", 
               wname, this->GetTclName());

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

  if (this->IsCreated())
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
  if (!this->IsCreated())
    {
    return;
    }

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
  
  this->GetApplication()->SimpleScript(str.str());
  str.rdbuf()->freeze(0);

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
  
  this->GetApplication()->SimpleScript(str.str());
  str.rdbuf()->freeze(0);

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
  this->GetApplication()->SimpleScript(str.str());
  str.rdbuf()->freeze(0);

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
  this->GetApplication()->SimpleScript(str.str());
  str.rdbuf()->freeze(0);

  if(!help)
    {
    help = label;
    }
  this->Script("set {%sHelpArray(%s)} {%s}", 
               this->GetTclName(), label, help);

  this->SetCascade(label, menu);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetCascadeIndex(vtkKWMenu* menu)
{
  if (menu && menu->IsCreated())
    {
    int nb_of_items = this->GetNumberOfItems();
    for (int i = 0; i < nb_of_items; i++)
      {
      const char *menu_opt = this->GetItemOption(i, "-menu");
      if (menu_opt && !strcmp(menu_opt, menu->GetWidgetName()))
        {
        return i;
        }
      }
    }

  return -1;
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
  str.rdbuf()->freeze(0);
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
  str.rdbuf()->freeze(0);
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
  str.rdbuf()->freeze(0);
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
  str.rdbuf()->freeze(0);
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
  char *buffer = NULL;
  const char *objname = Object->GetTclName();
  if (objname && varname)
    {
    buffer = new char[strlen(objname) + strlen(varname) + 1]; 
    sprintf(buffer, "%s%s", objname, varname);
    }
  return buffer;
}
  
//----------------------------------------------------------------------------
int vtkKWMenu::GetRadioButtonValue(vtkKWObject* Object, 
                                   const char* varname)
{
  int res;
  
  char *rbv = 
    this->CreateRadioButtonVariable(Object,varname);
  res = atoi(this->Script("set %s",rbv));
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
        ;
        if (atoi(
              this->Script("%s entrycget %i -value", 
                           this->GetWidgetName(), i)) == value)
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
  if (atoi(this->Script("set %s",rbv)) != id)
    {
    this->Script("set %s %d",rbv,id);
    }
  delete [] rbv;
}

//----------------------------------------------------------------------------
char* vtkKWMenu::CreateCheckButtonVariable(vtkKWObject* Object, 
                                           const char* varname)
{
  return this->CreateRadioButtonVariable(Object, varname);
}
  
//----------------------------------------------------------------------------
int vtkKWMenu::GetCheckButtonValue(vtkKWObject* Object, 
                                   const char* name)
{
  int res;
  
  char *rbv = 
    this->CreateCheckButtonVariable(Object,name);
  res = atoi(this->Script("set %s",rbv));
  delete [] rbv;
  return res;
}
    
//----------------------------------------------------------------------------
void vtkKWMenu::CheckCheckButton(vtkKWObject* Object, 
                                 const char* name, int id)
{
  char *rbv = 
    this->CreateCheckButtonVariable(Object,name);
  if (atoi(this->Script("set %s",rbv)) != id)
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
  str.rdbuf()->freeze(0);
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
  str.rdbuf()->freeze(0);
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
  str.rdbuf()->freeze(0);
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
  const char *wname = this->GetWidgetName();
  this->Script(
    "catch {%s delete %d} ; set {%sHelpArray([%s entrycget %d -label])} {}", 
    wname, position, 
    wname, wname, position);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteMenuItem(const char* menuitem)
{
  const char *wname = this->GetWidgetName();
  this->Script("catch {%s delete {%s}} ; set {%sHelpArray(%s)} {}",
               wname, menuitem, wname, menuitem);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteAllMenuItems()
{
  int nb_items = this->GetNumberOfItems();
  if (!nb_items)
    {
    return;
    }

  ostrstream tk_cmd;
  const char *wname = this->GetWidgetName();

  for (int i = nb_items - 1; i >= 0; --i)
    {
    tk_cmd << "catch {" << wname << " delete " << i << "}" << endl
           << "set {" << wname << "HelpArray([" 
           << wname << " entrycget " << i << " -label])} {}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndex(const char* menuname)
{
  return atoi(this->Script("%s index {%s}", this->GetWidgetName(), menuname));
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfCommand(
  vtkKWObject* Object, const char* MethodAndArgString)
{
  if (Object && Object->GetApplication() && MethodAndArgString)
    {
    ostrstream str;
    str << Object->GetTclName() << " " << MethodAndArgString << ends;
    int nb_of_items = this->GetNumberOfItems();
    for (int i = 0; i < nb_of_items; i++)
      {
      const char *command_opt = this->GetItemOption(i, "-command");
      if (command_opt && !strcmp(command_opt, str.str()))
        {
        str.rdbuf()->freeze(0);
        return i;
        }
      }
    str.rdbuf()->freeze(0);
    }

  return -1;
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
  if (this->IsCreated())
    {
    return !atoi(
      this->Script("catch {%s index {%s}}", this->GetWidgetName(), menuname));
    }
  return 0;
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
  if (this->IsCreated())
    {
    char stateStr[][9] = { "normal", "active", "disabled" };
    if (state < vtkKWMenu::Normal || state > vtkKWMenu::Disabled)
      {
      state = 0;
      }
    this->Script("catch {%s entryconfigure %d -state %s}", 
                 this->GetWidgetName(), index, stateStr[state] );
    }
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
  int nb_items = this->GetNumberOfItems();
  if (!nb_items)
    {
    return;
    }

  ostrstream tk_cmd;
  const char *wname = this->GetWidgetName();

  char stateStr[][9] = { "normal", "active", "disabled" };
  if (state < vtkKWMenu::Normal || state > vtkKWMenu::Disabled)
    {
    state = 0;
    }

  for (int i = 0; i < nb_items; i++)
    {
    tk_cmd << "catch {" << wname << " entryconfigure " << i 
           << " -state " << stateStr[state] << "}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
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
int vtkKWMenu::HasItemOption(int idx, const char *option)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return 0;
    }
 
  return !this->GetApplication()->EvaluateBooleanExpression(
    "catch {%s entrycget %d %s}",
    this->GetWidgetName(), idx, option);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemOption(int idx, const char *option)
{
  if (!this->HasItemOption(idx, option))
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
  this->Script("%s entryconfigure %d -compound left -image %s -hidemargin 1", 
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

