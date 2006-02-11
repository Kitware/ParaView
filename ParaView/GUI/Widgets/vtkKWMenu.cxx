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

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWWindowBase.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWIcon.h"

#include <ctype.h>

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMenu );
vtkCxxRevisionMacro(vtkKWMenu, "1.91");

//----------------------------------------------------------------------------
vtkKWMenu::vtkKWMenu()
{
  this->TearOff = 0;
}

//----------------------------------------------------------------------------
vtkKWMenu::~vtkKWMenu()
{
}

//----------------------------------------------------------------------------
void vtkKWMenu::Create()
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::CreateSpecificTkWidget("menu"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetConfigurationOptionAsInt("-tearoff", this->TearOff);
  this->SetBinding("<<MenuSelect>>", this, "DisplayHelp %W");

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

  this->SetConfigurationOptionAsInt("-tearoff", this->TearOff);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DisplayHelp(const char* widget)
{
  const char* tname = this->GetTclName();
  const char * res = this->Script(
    "if [catch {set %sTemp $%sHelpArray([%s entrycget active -label])} %sTemp ]"
    " { set %sTemp \"\"}; set %sTemp", 
    tname, tname, widget, tname, tname, tname );
  if(res)
    {
    vtkKWWindowBase* window = this->GetParentWindow();
    if ( window )
      {
      window->SetStatusText(res);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddGeneric(const char* addtype, 
                           const char* label,
                           vtkObject *object,
                           const char *method,
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

  if (object || method)
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    str << " -command {" << command << "}" ;
    delete [] command;
    }

  if(extra)
    {
    str << " " << extra;
    }

  str << ends;
  
  this->Script(str.str());
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
                              vtkObject *object,
                              const char *method, 
                              const char* extra, 
                              const char* help)
{
  ostrstream str;
  str << this->GetWidgetName() << " insert " << position << " " << addtype;

  if (label)
    {
    str << " -label {" << label << "}";
    }

  if (object || method)
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);
    str << " -command {" << command << "}" ;
    delete [] command;
    }

  if(extra)
    {
    str << " " << extra;
    }

  str << ends;
  
  this->Script(str.str());
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
  this->Script(str.str());
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
  this->Script(str.str());
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
void vtkKWMenu::SetCascade(int index, const char* menu_name)
{
  if (!menu_name)
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
  int child_length = (int)(strlen(menu_name));

  if (child_length < (parent_length + 2) || 
      strncmp(wname, menu_name, parent_length) ||
      menu_name[parent_length] != '.')
    {
    ostrstream clone_menu;
    clone_menu << wname << ".clone_";
    const char *res = 
      this->Script("string trim [%s entrycget %d -label]",  wname, index);
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
                 clone_menu.str(), menu_name, clone_menu.str());
    str << " -menu {" << clone_menu.str() << "}" << ends;
    clone_menu.rdbuf()->freeze(0); 
    }
  else
    {
    str << " -menu {" << menu_name << "}" << ends;
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
void vtkKWMenu::SetCascade(const char *label, vtkKWMenu* menu)
{
  if (!menu || !this->HasItem(label))
    {
    return;
    }
  this->SetCascade(this->GetIndexOfItem(label), menu->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetCascade(const char *label, const char* menu_name)
{
  if (!menu_name || !this->HasItem(label))
    {
    return;
    }
  this->SetCascade(this->GetIndexOfItem(label), menu_name);
}

//----------------------------------------------------------------------------
void  vtkKWMenu::AddCheckButton(const char* label, const char* ButtonVar, 
                                vtkObject *object, 
                                const char *method, 
                                const char* help )
{ 
  this->AddCheckButton(label, ButtonVar, object, method, -1, help);
}
 
//----------------------------------------------------------------------------
void  vtkKWMenu::AddCheckButton(const char* label, const char* ButtonVar, 
                                vtkObject *object, 
                                const char *method, 
                                int underline, const char* help )
{ 
  ostrstream str;
  str << "-variable " << ButtonVar;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->AddGeneric("checkbutton", label, object, 
                   method, str.str(), help);
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void vtkKWMenu::InsertCheckButton(int position, 
                                  const char* label, const char* ButtonVar, 
                                  vtkObject *object, 
                                  const char *method, const char* help )
{ 
  this->InsertCheckButton( position, label, ButtonVar, object, method,
                           -1, help );
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertCheckButton(int position, 
                                  const char* label, const char* ButtonVar, 
                                  vtkObject *object, 
                                  const char *method, 
                                  int underline, const char* help )
{ 
  ostrstream str;
  str << "-variable " << ButtonVar;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->InsertGeneric(position, "checkbutton", label, object, 
                      method, str.str(), help);
  str.rdbuf()->freeze(0);
}


//----------------------------------------------------------------------------
void  vtkKWMenu::AddCommand(const char* label, vtkObject *object,
                            const char *method,
                            int underline, 
                            const char* help)
{
  ostrstream str;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->AddGeneric("command", label, object, 
                   method, str.str(), help);
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void  vtkKWMenu::AddCommand(const char* label, vtkObject *object,
                            const char *method ,
                            const char* help)
{
  this->AddGeneric("command", label, object, 
                   method, NULL, help);
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertCommand(int position, const char* label, vtkObject *object,
                              const char *method,
                              int underline, 
                              const char* help)
{
  ostrstream str;
  if ( underline >= 0 )
    {
    str << " -underline " << underline;
    }
  str << ends;
  this->InsertGeneric(position, "command", label, object,
                      method, str.str(), help);
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertCommand(int position, const char* label, vtkObject *object,
                              const char *method,
                              const char* help)
{
  this->InsertGeneric(position, "command", label, object,
                      method, NULL, help);
}

//----------------------------------------------------------------------------
char* vtkKWMenu::CreateRadioButtonVariable(vtkKWObject* object, 
                                           const char* varname)
{
  char *buffer = NULL;
  const char *objname = object->GetTclName();
  if (objname && varname)
    {
    char *clean_name = vtksys::SystemTools::RemoveChars(varname, " ");
    buffer = new char[strlen(objname) + strlen(clean_name) + 1]; 
    sprintf(buffer, "%s%s", objname, clean_name);
    delete [] clean_name;
    }
  return buffer;
}
  
//----------------------------------------------------------------------------
int vtkKWMenu::GetRadioButtonValue(vtkKWObject* object, 
                                   const char* varname)
{
  int res;
  
  char *rbv = 
    this->CreateRadioButtonVariable(object,varname);
  res = atoi(this->Script("set %s",rbv));
  delete [] rbv;
  return res;
}
    
//----------------------------------------------------------------------------
int vtkKWMenu::GetCheckedRadioButtonItem(vtkKWObject* object, 
                                         const char* varname)
{
  char *rbv = this->CreateRadioButtonVariable(object,varname);
  int value = this->GetCheckButtonValue(object,varname);

  int numEntries = this->GetNumberOfItems();
  for(int i = 0; i < numEntries; i++)
    {
    const char *res = this->Script("%s type %d", this->GetWidgetName(), i);
    if (!strcmp("radiobutton", res))
      {
      res = 
        this->Script("%s entrycget %i -variable", this->GetWidgetName(), i);
      if (!strcmp(rbv, res))
        {
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
void vtkKWMenu::CheckRadioButton(vtkKWObject* object, 
                                 const char* varname, int id)
{
  char *rbv = this->CreateRadioButtonVariable(object,varname);
  this->Script("if {![info exists %s] || $%s != %d} {set %s %d}",
               rbv, rbv, id, rbv, id);
  delete [] rbv;
}

//----------------------------------------------------------------------------
char* vtkKWMenu::CreateCheckButtonVariable(vtkKWObject* object, 
                                           const char* varname)
{
  return this->CreateRadioButtonVariable(object, varname);
}
  
//----------------------------------------------------------------------------
int vtkKWMenu::GetCheckButtonValue(vtkKWObject* object, 
                                   const char* name)
{
  int res;
  
  char *rbv = 
    this->CreateCheckButtonVariable(object,name);
  res = atoi(this->Script("set %s",rbv));
  delete [] rbv;
  return res;
}
    
//----------------------------------------------------------------------------
void vtkKWMenu::CheckCheckButton(vtkKWObject* object, 
                                 const char* name, int id)
{
  char *rbv = this->CreateCheckButtonVariable(object,name);
  this->Script("if {![info exists %s] || $%s != %d} {set %s %d}",
               rbv, rbv, id, rbv, id);
  delete [] rbv;
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddRadioButton(int value, 
                               const char* label, 
                               const char* buttonVar, 
                               vtkObject *object, 
                               const char *method,
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
  this->AddGeneric("radiobutton", label, object,
                   method, str.str(), help);
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::AddRadioButton(int value, const char* label, const char* buttonVar, 
                               vtkObject *object, 
                               const char *method,
                               const char* help)
{
  this->AddRadioButton(value, label, buttonVar, object, method,
                       -1, help);
}


//----------------------------------------------------------------------------
void vtkKWMenu::AddRadioButtonImage(int value, 
                                    const char* imgname, 
                                    const char* buttonVar, 
                                    vtkObject *object, 
                                    const char *method,
                                    const char* help)
{
  ostrstream str;
  str << "-image " << imgname 
      << " -selectimage " << imgname 
      << " -value " << value 
      << " -variable " << buttonVar
      << ends;
  // Uses the imgname as label, so that the help string can work.
  this->AddGeneric("radiobutton", imgname, object,
                   method, str.str(), help);
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertRadioButton(int position, int value, const char* label, 
                                  const char* buttonVar, 
                                  vtkObject *object, 
                                  const char *method,
                                  const char* help)
{
  this->InsertRadioButton( position, value, label, buttonVar, object,
                           method, -1, help );
}

//----------------------------------------------------------------------------
void vtkKWMenu::InsertRadioButton(int position, int value, const char* label, 
                                  const char* buttonVar, 
                                  vtkObject *object, 
                                  const char *method,
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
  this->InsertGeneric(position, "radiobutton", label, object,
                      method, str.str(), help);
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::Invoke(int position)
{
  this->Script("%s invoke %d", this->GetWidgetName(), position);
}

//----------------------------------------------------------------------------
void vtkKWMenu::Invoke(const char *label)
{
  if (!this->HasItem(label))
    {
    return;
    }
  this->Invoke(this->GetIndexOfItem(label));
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
void vtkKWMenu::DeleteMenuItem(const char *label)
{
  if (!this->HasItem(label))
    {
    return;
    }
  this->DeleteMenuItem(this->GetIndexOfItem(label));
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
int vtkKWMenu::GetNumberOfItems()
{
  if (this->IsAlive())
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
int vtkKWMenu::HasItem(const char *label)
{
  return this->GetIndexOfItem(label) >= 0 ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfItem(const char *label)
{
  // This one is tricky
  // Calling 'index' only works if the parameter is not a number, or
  // not any of 'active', 'end', 'last', 'none' or '@number', which are
  // interpreted differently. Detect that, and loop over all entries if
  // required

  if (!label || !*label)
    {
    return -1;
    }

  // Check if it is a number

  const char *ptr = label;
  while (*ptr && isdigit(*ptr))
    {
    ++ptr;
    }

  // If it is not a number, and it is not of the special keyword, use 'index'

  if (*ptr &&
      strcmp(label, "active") &&
      strcmp(label, "end") &&
      strcmp(label, "last") &&
      strcmp(label, "none") &&
      *label != '@')
    {
    int not_ok = atoi(
      this->Script("catch {%s index {%s}} %s_getindex", 
                   this->GetWidgetName(), label, this->GetTclName()));
    if (not_ok)
      {
      return -1;
      }
    return atoi(this->Script("set %s_getindex", this->GetTclName()));
    }

  // OK, it is either a number or one of the special keywords, check manually

  int nb_of_items = this->GetNumberOfItems();
  for (int i = 0; i < nb_of_items; i++)
    {
    const char *label_opt = this->GetItemOption(i, "-label");
    if (label_opt && *label_opt && !strcmp(label_opt, label))
      {
      return i;
      }
    }

  return -1;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfCommand(
  vtkObject *object, const char *method)
{
  if (object || method)
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);

    int nb_of_items = this->GetNumberOfItems();
    for (int i = 0; i < nb_of_items; i++)
      {
      const char *command_opt = this->GetItemOption(i, "-command");
      if (command_opt && !strcmp(command_opt, command))
        {
        delete [] command;
        return i;
        }
      }
    delete [] command;
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
int vtkKWMenu::GetItemState(int index)
{
  const char *state = 
    this->Script("%s entrycget %d -state", this->GetWidgetName(), index);
  return vtkKWTkOptions::GetStateFromTkOptionValue(state);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemState(const char *label)
{
  if (!this->HasItem(label))
    {
    return vtkKWTkOptions::StateUnknown;
    }
  return this->GetItemState(this->GetIndexOfItem(label));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemState(int index, int state)
{
  if (this->IsCreated())
    {
    this->Script("catch {%s entryconfigure %d -state %s}", 
                 this->GetWidgetName(), 
                 index, 
                 vtkKWTkOptions::GetStateAsTkOptionValue(state));
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemState(const char *label, int state)
{
  if (!this->HasItem(label))
    {
    return;
    }
  this->SetItemState(this->GetIndexOfItem(label), state);
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

  const char *statestr = vtkKWTkOptions::GetStateAsTkOptionValue(state);

  for (int i = 0; i < nb_items; i++)
    {
    tk_cmd << "catch {" << wname << " entryconfigure " << i 
           << " -state " << statestr << "}" << endl;
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
void vtkKWMenu::SetEntryCommand(int index, vtkObject *object, 
                           const char *method)
{
  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);
  this->SetEntryCommand(index, command);
  delete [] command;
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(int idx, const char *method)
{
  if ( !this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems() )
    {
    return;
    }
  ostrstream str;
  str << this->GetWidgetName() << " entryconfigure "
      << idx << " -command {" << method << "}" << ends;
  this->Script(str.str());
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(const char *label, const char *method)
{
  if (!this->HasItem(label))
    {
    return;
    }
  this->SetEntryCommand(this->GetIndexOfItem(label), method);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEntryCommand(const char *label, vtkObject *object, 
                           const char *method)
{
  if ( !this->HasItem(label))
    {
    return;
    }
  this->SetEntryCommand(
    this->GetIndexOfItem(label), object, method);
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
const char* vtkKWMenu::GetItemOption(const char *label, const char *option)
{
  return this->GetItemOption(this->GetIndexOfItem(label), option);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemCommand(int idx)
{
  return this->GetItemOption(idx, "-command");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemImage(int idx, const char *imagename)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  
  // -image is not supported on MacOS Aqua system

  int tcl_major, tcl_minor, tcl_patch_level;
  Tcl_GetVersion(&tcl_major, &tcl_minor, &tcl_patch_level, NULL);
  if (tcl_major < 8 ||
      (tcl_major == 8 && 
       (tcl_minor < 4 || 
        (tcl_minor == 4 && tcl_patch_level <= 12))))
    {
    vtksys_stl::string sys(
      vtkKWTkUtilities::GetWindowingSystem(this->GetApplication()));
    if (!sys.compare("aqua"))
      {
      return;
      }
    }

  this->Script("%s entryconfigure %d -image %s", 
               this->GetWidgetName(), idx, imagename);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemImage(const char *label, const char *imagename)
{
  this->SetItemImage(this->GetIndexOfItem(label), imagename);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemImageToPredefinedIcon(int idx, int icon_index)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }

  char buffer[1024];

  sprintf(buffer, "%s.PredefinedIcon%d", this->GetTclName(), icon_index);
  if (!vtkKWTkUtilities::FindPhoto(this->GetApplication(), buffer))
    {
    vtkKWTkUtilities::UpdatePhotoFromPredefinedIcon(
      this->GetApplication(), buffer, icon_index);
    }

#if 0
  this->SetItemSelectImage(idx, buffer);

  sprintf(buffer, "%s.PredefinedIconFaded%d", this->GetTclName(), icon_index);
  if (!vtkKWTkUtilities::FindPhoto(this->GetApplication(), buffer))
    {
    vtkKWIcon *icon_faded = vtkKWIcon::New();
    icon_faded->SetImage(icon_index);
    icon_faded->Fade(0.3);
    
    vtkKWTkUtilities::UpdatePhotoFromIcon(
      this->GetApplication(), buffer, icon_faded);
    icon_faded->Delete();
    }
  this->SetItemIndicatorVisibility(idx, 0);
#endif

  this->SetItemImage(idx, buffer);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemImageToPredefinedIcon(const char *label, int icon_index)
{
  this->SetItemImageToPredefinedIcon(this->GetIndexOfItem(label), icon_index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectImage(int idx, const char *imagename)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -selectimage %s", 
               this->GetWidgetName(), idx, imagename);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectImage(const char *label, const char *imagename)
{
  this->SetItemSelectImage(this->GetIndexOfItem(label), imagename);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectImageToPredefinedIcon(int idx, int icon_index)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }

  char buffer[1024];
  sprintf(buffer, "%s.PredefinedIcon%d", this->GetTclName(), icon_index);
  if (!vtkKWTkUtilities::FindPhoto(this->GetApplication(), buffer))
    {
    vtkKWTkUtilities::UpdatePhotoFromPredefinedIcon(
      this->GetApplication(), buffer, icon_index);
    }
  this->SetItemSelectImage(idx, buffer);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectImageToPredefinedIcon(
  const char *label, int icon_index)
{
  this->SetItemSelectImageToPredefinedIcon(
    this->GetIndexOfItem(label), icon_index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCompoundMode(int idx, int flag)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -compound %s", 
               this->GetWidgetName(), idx, (flag ? "left" : "none"));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCompoundMode(const char *label, int mode)
{
  this->SetItemCompoundMode(this->GetIndexOfItem(label), mode);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemMarginVisibility(int idx, int flag)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -hidemargin %d", 
               this->GetWidgetName(), idx, flag ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemMarginVisibility(const char *label, int flag)
{
  this->SetItemMarginVisibility(this->GetIndexOfItem(label), flag);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemIndicatorVisibility(int idx, int flag)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -indicatoron %d", 
               this->GetWidgetName(), idx, flag ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemIndicatorVisibility(const char *label, int flag)
{
  this->SetItemIndicatorVisibility(this->GetIndexOfItem(label), flag);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemAccelerator(int idx, const char *accelerator)
{
  if (!this->IsCreated() || idx < 0 || idx >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -accelerator {%s}", 
               this->GetWidgetName(), idx, accelerator);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemAccelerator(const char *label, const char *accelerator)
{
  this->SetItemAccelerator(this->GetIndexOfItem(label), accelerator);
}

//----------------------------------------------------------------------------
void vtkKWMenu::PopUp(int x, int y)
{
  if (this->IsCreated())
    {
    this->Script("tk_popup %s %d %d", this->GetWidgetName(), x, y);
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetEnabled(int e)
{
  int old_enabled = this->GetEnabled();
  this->Superclass::SetEnabled(e);

  // So even if the requested state was the same, propagate to the entries

  if (this->GetEnabled() == old_enabled)
    {
    this->UpdateEnableState();
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->SetState(this->GetEnabled());
}

//----------------------------------------------------------------------------
void vtkKWMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TearOff: " << this->GetTearOff() << endl;
}

