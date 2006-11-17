y/*=========================================================================

  Module:    vtkKWMenu.cxx,v

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
#include "vtkKWOptions.h"

#include <ctype.h>

#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/string>
#include <vtksys/stl/map>

#define VTK_KW_MENU_CB_VARNAME_PATTERN "CB_group%d"
#define VTK_KW_MENU_RB_DEFAULT_GROUP "RB_group"
#define VTK_KW_MENU_VAR_SEPARATOR "_"

//----------------------------------------------------------------------------
vtkStandardNewMacro( vtkKWMenu );
vtkCxxRevisionMacro(vtkKWMenu, "1.109");

//----------------------------------------------------------------------------
class vtkKWMenuInternals
{
public:
  int ItemCounter;
  int IndexOfLastActiveItem;
  vtksys_stl::string GetItemCommandTemp;

  struct CascadePoolType: public vtksys_stl::map<vtksys_stl::string, vtkKWMenu*> {};
  CascadePoolType CascadePool;
};

//----------------------------------------------------------------------------
vtkKWMenu::vtkKWMenu()
{
  this->TearOff = 0;

  this->Internals = new vtkKWMenuInternals;
  this->Internals->ItemCounter = 0;
  this->Internals->IndexOfLastActiveItem = -1;
}

//----------------------------------------------------------------------------
vtkKWMenu::~vtkKWMenu()
{
  if (this->Internals)
    {
    delete this->Internals;
    this->Internals = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::CreateWidget()
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!vtkKWWidget::CreateSpecificTkWidget(this, "menu"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  this->SetConfigurationOptionAsInt("-tearoff", this->TearOff);
  this->SetBinding("<<MenuSelect>>", this, "DisplayHelpCallback %W");
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetLabelWithoutUnderline(
  const char *label, char **clean_label, int *underline_index)
{
  const char marker = '&';

  // Find the marker (should not be the last char, or followed by space)

  char *found = const_cast<char*>(strchr(label, marker));
  while(found)
    {
    ++found;
    if(*found && *found != ' ')
      {
      break;
      }
    found = const_cast<char*>(strchr(const_cast<const char*>(found), marker));
    }

  // Not found, sorry

  if (!found)
    {
    *clean_label = const_cast<char*>(label);
    *underline_index = -1;
    return 0;
    }

  // Create the clean one

  *underline_index = found - label - 1;
  size_t clean_label_len = *underline_index + strlen(found);
  *clean_label = new char [clean_label_len + 1];
  if (*underline_index)
    {
    memcpy(*clean_label, label, *underline_index); // contents before marker
    }
  memcpy(*clean_label + *underline_index, const_cast<const char*>(found), 
         clean_label_len - *underline_index); // contents after marker
  (*clean_label)[clean_label_len] = '\0';

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddGeneric(const char* type, 
                          const char *label,
                          const char* extra)
{
  return this->InsertGeneric(this->GetNumberOfItems(), type, label, extra);
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertGeneric(int index, 
                             const char* type, 
                             const char *label, 
                             const char* extra)
{
  if (!this->IsCreated())
    {
    return -1;
    }

  if (index < 0)
    {
    index = 0;
    }
  if (index == 0)
    {
    if (this->TearOff) // If tearoff, then entry 0 is always taken
      {
      index = 1;
      }
    } 
  else
    {
    int nb_items = this->GetNumberOfItems(); // cap the index
    if (index > nb_items)
      {
      index = nb_items;
      }
    }

  ostrstream str;
  str << this->GetWidgetName() << " insert " << index << " " << type;

  char *clean_label = NULL;
  int underline_index, cleaned = 0;
  if (label)
    {
    cleaned = 
      this->GetLabelWithoutUnderline(label, &clean_label, &underline_index);
    str << " -label {" << clean_label << "}";
    }

  if(extra)
    {
    str << " " << extra;
    }

  str << ends;
  
  this->Script(str.str());
  str.rdbuf()->freeze(0);

  if (label)
    {
    this->SetItemHelpString(index, clean_label);
    if (cleaned)
      {
      this->SetItemUnderline(index, underline_index);
      delete [] clean_label;
      }
    }

  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddCommand(const char *label)
{
  return this->AddCommand(label, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddCommand(const char *label, 
                          vtkObject *object, const char *method)
{
  int index = this->AddGeneric("command", label, NULL);
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    this->InvokeEvent(vtkKWMenu::CommandItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertCommand(int index, const char *label)
{
  return this->InsertCommand(index, label, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertCommand(int index, 
                             const char *label, 
                             vtkObject *object, const char *method)
{
  index = this->InsertGeneric(index, "command", label, NULL);
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    this->InvokeEvent(vtkKWMenu::CommandItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCommand(int index, 
                               vtkObject *object, 
                               const char *method)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }

  char *command = NULL;
  this->SetObjectMethodCommand(&command, object, method);

  // Let's add the CommandInvokedCallback wrapper
  // The wrapper is used to send an event when the entry is invoked.

  this->Script(
    "%s entryconfigure %d -command {%s CommandInvokedCallback {%s}}", 
    this->GetWidgetName(), index, this->GetTclName(), command);

  delete [] command;

  this->InstallItemAcceleratorBindingOnToplevel(index);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemCommand(int index)
{
  const char *command = this->GetItemOption(index, "-command");
  if (!command || !*command)
    {
    return command;
    }

  // Let's strip the CommandInvokedCallback wrapper out
  // The wrapper is used to send an event when the entry is invoked.

  vtksys_stl::string &temp = this->Internals->GetItemCommandTemp;
  temp = command;

  const char *pattern = "CommandInvokedCallback {";
  vtksys_stl::string::size_type pos = temp.find(pattern);
  if(pos != vtksys_stl::string::npos)
    {
    temp[temp.size() - 1] = '\0'; // Remove the last enclosing '}'
    return temp.c_str() + pos + strlen(pattern);
    }

  return command;
}

//----------------------------------------------------------------------------
void vtkKWMenu::CommandInvokedCallback(const char *command)
{
  int index = this->Internals->IndexOfLastActiveItem;
  if (index >= 0)
    {
    this->InvokeEvent(vtkKWMenu::MenuItemInvokedEvent, &index);
    }
  this->InvokeObjectMethodCommand(command);
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddCheckButton(const char *label)
{ 
  return this->AddCheckButton(label, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddCheckButton(const char *label,
                              vtkObject *object, const char *method)
{ 
  int index = this->AddGeneric("checkbutton", label, NULL);
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    char group_name[200];
    sprintf(group_name, VTK_KW_MENU_CB_VARNAME_PATTERN, 
            this->Internals->ItemCounter++);
    this->SetItemVariable(index, this, group_name);
    this->SetItemSelectedValueAsInt(index, 1);
    this->SetItemDeselectedValueAsInt(index, 0);
    this->InvokeEvent(vtkKWMenu::CheckButtonItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertCheckButton(int index, 
                                 const char *label)
{ 
  return this->InsertCheckButton(index, label, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertCheckButton(int index, 
                                 const char *label,
                                 vtkObject *object, const char *method)
{ 
  index = this->InsertGeneric(index, "checkbutton", label, NULL);
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    char group_name[200];
    sprintf(group_name, VTK_KW_MENU_CB_VARNAME_PATTERN, 
            this->Internals->ItemCounter++);
    this->SetItemVariable(index, this, group_name);
    this->SetItemSelectedValueAsInt(index, 1);
    this->SetItemDeselectedValueAsInt(index, 0);
    this->InvokeEvent(vtkKWMenu::CheckButtonItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
void vtkKWMenu::SelectItem(int index)
{
  const char *temp = this->GetItemVariable(index);
  if (temp)
    {
    vtksys_stl::string varname(temp);
    this->SetItemVariableValue(
      varname.c_str(), this->GetItemSelectedValue(index));
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::SelectItem(const char *label)
{
  this->SelectItem(this->GetIndexOfItem(label));
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeselectItem(int index)
{
  const char *temp = this->GetItemVariable(index);
  if (temp)
    {
    vtksys_stl::string varname(temp);
    this->SetItemVariableValue(
      varname.c_str(), this->GetItemDeselectedValue(index));
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeselectItem(const char *label)
{
  this->DeselectItem(this->GetIndexOfItem(label));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectedState(int index, int state)
{
  if (state)
    {
    this->SelectItem(index);
    }
  else
    {
    this->DeselectItem(index);
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectedState(const char *label, int state)
{
  this->SetItemSelectedState(this->GetIndexOfItem(label), state);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemSelectedState(int index)
{
  const char *temp = this->GetItemVariableValue(this->GetItemVariable(index));
  if (temp)
    {
    vtksys_stl::string current_val(temp);
    temp = this->GetItemSelectedValue(index);
    if (temp)
      {
      return !strcmp(current_val.c_str(), temp) ? 1 : 0;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemSelectedState(const char *label)
{
  return this->GetItemSelectedState(this->GetIndexOfItem(label));
}

//----------------------------------------------------------------------------
char* vtkKWMenu::CreateItemVariableName(vtkKWObject *object, 
                                        const char *suffix)
{
  char *buffer = NULL;
  const char *objname = object->GetTclName();
  if (objname && suffix)
    {
    size_t objname_len = strlen(objname);
    size_t suffix_len = strlen(suffix);
    size_t sep_len = strlen(VTK_KW_MENU_VAR_SEPARATOR);
    buffer = new char[objname_len + sep_len + suffix_len + 1]; 
    sprintf(buffer, "%s%s", objname, VTK_KW_MENU_VAR_SEPARATOR);
    char *buffer_ptr = buffer + objname_len + sep_len;
    const char *suffix_end = suffix + suffix_len;
    while (suffix < suffix_end)
      {
      if (*suffix >= 0 && *suffix != ' ')
        {
        *buffer_ptr = *suffix;
        buffer_ptr++;
        }
      suffix++;
      }
    *buffer_ptr = '\0';
    }
  return buffer;
}
  
//----------------------------------------------------------------------------
const char* vtkKWMenu::GetSuffixOutOfCreatedItemVariableName(
  const char *varname)
{
  if (varname)
    {
    return varname + 
      strlen(this->GetTclName()) + 
      strlen(VTK_KW_MENU_VAR_SEPARATOR);
    }
  return NULL;
}
  
//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemVariableValue(const char *varname)
{
  if (varname && *varname)
    {
    return this->Script("set %s", varname);
    }
  return NULL;
}
    
//----------------------------------------------------------------------------
void vtkKWMenu::SetItemVariableValue(const char *varname, const char *value)
{
  this->Script("if {![info exists %s] || \"$%s\" ne \"%s\"} {set %s \"%s\"}",
               varname, varname, value, varname, value);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemVariableValueAsInt(const char *varname)
{
  const char *temp = this->GetItemVariableValue(varname);
  if (temp)
    {
    return atoi(temp);
    }
  return 0;
}
    
//----------------------------------------------------------------------------
void vtkKWMenu::SetItemVariableValueAsInt(const char *varname, int value)
{
  this->Script("if {![info exists %s] || $%s != %d} {set %s %d}",
               varname, varname, value, varname, value);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemVariable(int index, const char *varname)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -variable {%s}", 
               this->GetWidgetName(), index, varname);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemVariable(int index, 
                                vtkKWObject *object, 
                                const char *suffix)
{
  char *varname = this->CreateItemVariableName(object, suffix);
  this->SetItemVariable(index, varname);
  delete [] varname;
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemVariable(int index)
{
  return this->GetItemOption(index, "-variable");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectedValue(int index, const char* value)
{
  if (!this->IsCreated())
    {
    return;
    }

  vtksys_stl::string value_safe(value ? value : "");

  if (index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }

  vtksys_stl::string type(
    this->Script("%s type %d", this->GetWidgetName(), index));
  if (!strcmp("radiobutton", type.c_str()))
    {
    this->Script("%s entryconfigure %d -value {%s}", 
                 this->GetWidgetName(), index, value_safe.c_str());
    }
  else if (!strcmp("checkbutton", type.c_str()))
    {
    this->Script("%s entryconfigure %d -onvalue {%s}", 
                 this->GetWidgetName(), index, value_safe.c_str());
    }
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemSelectedValue(int index)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return NULL;
    }

  vtksys_stl::string type(
    this->Script("%s type %d", this->GetWidgetName(), index));
  if (!strcmp("radiobutton", type.c_str()))
    {
    return this->GetItemOption(index, "-value");
    }
  else if (!strcmp("checkbutton", type.c_str()))
    {
    return this->GetItemOption(index, "-onvalue");
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectedValueAsInt(int index, int value)
{
  char buffer[50];
  sprintf(buffer, "%d", value);
  this->SetItemSelectedValue(index, buffer);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemSelectedValueAsInt(int index)
{
  return atoi(this->GetItemSelectedValue(index));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemDeselectedValue(int index, const char* value)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }

  this->Script("%s entryconfigure %d -offvalue {%s}", 
               this->GetWidgetName(), index, value ? value : "");
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemDeselectedValue(int index)
{
  return this->GetItemOption(index, "-offvalue");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemDeselectedValueAsInt(int index, int value)
{
  char buffer[50];
  sprintf(buffer, "%d", value);
  this->SetItemDeselectedValue(index, buffer);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemDeselectedValueAsInt(int index)
{
  return atoi(this->GetItemDeselectedValue(index));
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddRadioButton(const char *label)
{
  return this->AddRadioButton(label, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddRadioButton(const char *label,
                              vtkObject *object, const char *method)
{
  int index = this->AddGeneric("radiobutton", label, NULL);
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    this->SetItemVariable(index, this, VTK_KW_MENU_RB_DEFAULT_GROUP);
    this->SetItemSelectedValue(index, label);
    this->InvokeEvent(vtkKWMenu::RadioButtonItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddRadioButtonImage(const char *imgname)
{
  return this->AddRadioButtonImage(imgname, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddRadioButtonImage(const char *imgname,
                                   vtkObject *object, const char *method)
{
  vtksys_stl::string str("-image ");
  str += imgname;
  str += " -selectimage ";
  str += imgname;
  // Uses the imgname as label, so that the help string can work.
  int index = this->AddGeneric("radiobutton", imgname, str.c_str());
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    this->SetItemVariable(index, this, VTK_KW_MENU_RB_DEFAULT_GROUP);
    this->SetItemSelectedValue(index, imgname);
    this->InvokeEvent(vtkKWMenu::RadioButtonItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertRadioButton(int index, 
                                 const char *label)
{
  return this->InsertRadioButton(index, label, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertRadioButton(int index, 
                                 const char *label,
                                 vtkObject *object, const char *method)
{
  index = this->InsertGeneric(index, "radiobutton", label, NULL);
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    this->SetItemVariable(index, this, VTK_KW_MENU_RB_DEFAULT_GROUP);
    this->SetItemSelectedValue(index, label);
    this->InvokeEvent(vtkKWMenu::RadioButtonItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertRadioButtonImage(int index, const char *imgname)
{
  return this->InsertRadioButtonImage(index, imgname, NULL, NULL);
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertRadioButtonImage(int index, const char *imgname,
                                      vtkObject *object, const char *method)
{
  vtksys_stl::string str("-image ");
  str += imgname;
  str += " -selectimage ";
  str += imgname;
  // Uses the imgname as label, so that the help string can work.
  index = this->InsertGeneric(index, "radiobutton", imgname, str.c_str());
  if (index >= 0)
    {
    this->SetItemCommand(index, object, method);
    this->SetItemVariable(index, this, VTK_KW_MENU_RB_DEFAULT_GROUP);
    this->SetItemSelectedValue(index, imgname);
    this->InvokeEvent(vtkKWMenu::RadioButtonItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
void vtkKWMenu::PutItemInGroup(int index, int index_g)
{
  this->SetItemGroupName(
    index, this->GetItemGroupName(index_g));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemGroupName(int index, const char *group_name)
{
  char *varname = this->CreateItemVariableName(this, group_name);
  this->SetItemVariable(index, varname);
  delete [] varname;
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemGroupName(int index)
{
  return this->GetSuffixOutOfCreatedItemVariableName(
    this->GetItemVariable(index));
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfItemUsingVariableAndSelectedValue(
  const char *varname, const char *selected_value)
{
  if (varname && selected_value)
    {
    vtksys_stl::string varname_safe(varname);
    vtksys_stl::string selected_value_safe(selected_value);

    int nb_of_items = this->GetNumberOfItems();
    for(int i = 0; i < nb_of_items; i++)
      {
      const char *temp = this->GetItemVariable(i);
      if (temp && !strcmp(varname_safe.c_str(), temp))
        {
        temp = this->GetItemSelectedValue(i);
        if (temp && !strcmp(temp, selected_value_safe.c_str()))
          {
          return i;
          }
        }
      }
    }
  
  return -1;
}
    
//----------------------------------------------------------------------------
int vtkKWMenu::SelectItemInGroupWithSelectedValue(
  const char *group_name, const char *selected_value)
{
  int index = -1;
  char *varname = this->CreateItemVariableName(this, group_name);
  if (varname)
    {
    index = this->GetIndexOfItemUsingVariableAndSelectedValue(
      varname, selected_value);
    if (index >= 0)
      {
      this->SelectItem(index);
      }
    delete [] varname;
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::SelectItemInGroupWithSelectedValueAsInt(
  const char *group_name, int selected_value)
{
  char buffer[50];
  sprintf(buffer, "%d", selected_value);
  return this->SelectItemInGroupWithSelectedValue(
    group_name, buffer);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfSelectedItemInGroup(const char *group_name)
{
  int index = -1;
  char *varname = this->CreateItemVariableName(this, group_name);
  const char *temp = this->GetItemVariableValue(varname);
  if (temp)
    {
    vtksys_stl::string varvalue(temp);
    index = this->GetIndexOfItemUsingVariableAndSelectedValue(
      varname, varvalue.c_str());
    }
  delete [] varname;
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddSeparator()
{
  int index = this->AddGeneric("separator", NULL, NULL);
  this->InvokeEvent(vtkKWMenu::SeparatorItemAddedEvent, &index);
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertSeparator(int index)
{
  index = this->InsertGeneric(index, "separator", NULL, NULL);
  this->InvokeEvent(vtkKWMenu::SeparatorItemAddedEvent, &index);
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::AddCascade(const char *label, 
                           vtkKWMenu* menu)
{
  int index = this->AddGeneric("cascade", label, NULL);
  if (index >= 0)
    {
    this->SetItemCascade(index, menu);
    this->InvokeEvent(vtkKWMenu::CascadeItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::InsertCascade(int index, 
                             const char *label, 
                             vtkKWMenu* menu)
{
  index = this->InsertGeneric(index, "cascade", label, NULL);
  if (index >= 0)
    {
    this->SetItemCascade(index, menu);
    this->InvokeEvent(vtkKWMenu::CascadeItemAddedEvent, &index);
    }
  return index;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfCascadeItem(vtkKWMenu *menu)
{
  if (menu && menu->IsCreated())
    {
    int i, nb_of_items = this->GetNumberOfItems();
    for (i = 0; i < nb_of_items; i++)
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
void vtkKWMenu::SetItemCascade(int index, const char *menu_name)
{
  if (!menu_name)
    {
    return;
    }

  vtksys_stl::string menu_name_safe(menu_name);
  const char *wname = this->GetWidgetName();

  ostrstream str;
  str << wname << " entryconfigure " << index;

  // The cascade menu has to be a child 
  // (i.e. the parent + '.' + at least a letter)
  // If not, clone it.

  int parent_length = (int)(strlen(wname));
  int child_length = (int)(menu_name_safe.size());

  if (child_length < (parent_length + 2) || 
      strncmp(wname, menu_name_safe.c_str(), parent_length) ||
      menu_name_safe[parent_length] != '.')
    {
    ostrstream clone_menu;
    clone_menu << wname << ".clone_";
    vtksys_stl::string res(
      this->Script("string trim [%s entrycget %d -label]",  wname, index));
    if (res.size())
      {
      clone_menu << res.c_str();
      }
    else
      {
      clone_menu << index;
      }
    clone_menu << ends;
    this->Script("catch { destroy %s } \n %s clone %s", 
                 clone_menu.str(), menu_name_safe.c_str(), clone_menu.str());
    str << " -menu {" << clone_menu.str() << "}" << ends;
    clone_menu.rdbuf()->freeze(0); 
    }
  else
    {
    str << " -menu {" << menu_name_safe.c_str() << "}" << ends;
    }


  this->Script(str.str());
  str.rdbuf()->freeze(0); 
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCascade(int index, vtkKWMenu *menu)
{
  if (menu)
    {
    this->SetItemCascade(index, menu->GetWidgetName());
    const char *menu_opt = this->GetItemOption(index, "-menu");
    if (menu_opt && *menu_opt && this->Internals)
      {
      this->Internals->CascadePool[menu_opt] = menu;
      }
    }
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWMenu::GetItemCascade(int index)
{
  const char *menu_opt = this->GetItemOption(index, "-menu");
  if (menu_opt && *menu_opt && this->Internals)
    {
    vtkKWMenuInternals::CascadePoolType::iterator it = 
      this->Internals->CascadePool.find(menu_opt);
    if (it != this->Internals->CascadePool.end())
      {
      return it->second;
      }
    }
  return NULL;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfItem(const char *label)
{
  // This one is tricky
  // Calling 'index' only works if the parameter is not a number, or
  // not any of 'active', 'end', 'last', 'none' or '@number', which are
  // interpreted differently. Detect that, and loop over all entries if
  // required

  if (!label || !*label || !this->IsCreated())
    {
    return -1;
    }

  // Check if it is a number

  const char *ptr = label;
  while (*ptr && *ptr > 0 && isdigit(*ptr))
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
    char *clean_label = NULL;
    int underline_index, cleaned =
      this->GetLabelWithoutUnderline(label, &clean_label, &underline_index);

    const char *res = 
      this->Script("catch {%s index {%s}} %s_getindex", 
                   this->GetWidgetName(), clean_label, this->GetTclName());
    if (cleaned)
      {
      delete [] clean_label;
      }
    
    if (!res || atoi(res))
      {
      return -1;
      }
    return atoi(this->Script("set %s_getindex", this->GetTclName()));
    }

  // OK, it is either a number or one of the special keywords, check manually


  int found = -1, nb_of_items = this->GetNumberOfItems();
  for (int i = 0; i < nb_of_items; i++)
    {
    const char *label_opt = this->GetItemOption(i, "-label");
    if (label_opt && *label_opt && !strcmp(label_opt, label))
      {
      found = i;
      break;
      }
    }

  return found;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfActiveItem(const char *widget_name)
{
  if (widget_name)
    {
    const char *index_active = this->Script("%s index active", widget_name);
    if (strcmp(index_active, "none"))
      {
      return atoi(index_active);
      }
    }
  return -1;
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetIndexOfCommandItem(
  vtkObject *object, const char *method)
{
  if (object || method)
    {
    char *command = NULL;
    this->SetObjectMethodCommand(&command, object, method);

    int nb_of_items = this->GetNumberOfItems();
    for (int i = 0; i < nb_of_items; i++)
      {
      const char *command_opt = this->GetItemCommand(i);
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
int vtkKWMenu::HasItem(const char *label)
{
  return this->GetIndexOfItem(label) >= 0 ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWMenu::SetItemLabel(int index, const char *label)
{
  if (this->IsCreated() && index >= 0 && index < this->GetNumberOfItems())
    {
    char *clean_label = NULL;
    int underline_index, cleaned =
      this->GetLabelWithoutUnderline(label, &clean_label, &underline_index);

    this->Script("%s entryconfigure %d -label {%s}", 
                 this->GetWidgetName(), index, clean_label);

    const char *help = this->GetItemHelpString(index);
    if(!help || !*help)
      {
      this->SetItemHelpString(index, clean_label);
      }

    if (cleaned)
      {
      this->SetItemUnderline(index, underline_index);
      delete [] clean_label;
      }
    return 1;
    }

  return 0;
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemLabel(int index)
{
  return this->GetItemOption(index, "-label");
}

//----------------------------------------------------------------------------
void vtkKWMenu::InvokeItem(int index)
{
  this->Script("%s invoke %d", this->GetWidgetName(), index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteItem(int index)
{
  this->SetItemAccelerator(index, NULL);
  const char *wname = this->GetWidgetName();
  this->Script(
    "catch {%s delete %d} ; set {%sHelpArray([%s entrycget %d -label])} {}", 
    wname, index, 
    wname, wname, index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DeleteAllItems()
{
  int nb_of_items = this->GetNumberOfItems();
  if (!nb_of_items)
    {
    return;
    }

  ostrstream tk_cmd;
  const char *wname = this->GetWidgetName();

  for (int i = nb_of_items - 1; i >= 0; --i)
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
int vtkKWMenu::GetItemState(int index)
{
  return vtkKWOptions::GetStateFromTkOptionValue(
    this->GetItemOption(index, "-state"));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemState(int index, int state)
{
  if (this->IsCreated())
    {
    this->Script("catch {%s entryconfigure %d -state %s}", 
                 this->GetWidgetName(), 
                 index, 
                 vtkKWOptions::GetStateAsTkOptionValue(state));
    }
}

void vtkKWMenu::SetItemStateToDisabled(int index) 
{ 
  this->SetItemState(index, vtkKWOptions::StateDisabled); 
};

void vtkKWMenu::SetItemStateToNormal(int index) 
{ 
  this->SetItemState(index, vtkKWOptions::StateNormal); 
};


//----------------------------------------------------------------------------
int vtkKWMenu::GetItemState(const char *label)
{
  return this->GetItemState(this->GetIndexOfItem(label));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemState(const char *label, int state)
{
  this->SetItemState(this->GetIndexOfItem(label), state);
}

void vtkKWMenu::SetItemStateToDisabled(const char *label) 
{ 
  this->SetItemState(label, vtkKWOptions::StateDisabled); 
};

void vtkKWMenu::SetItemStateToNormal(const char *label) 
{ 
  this->SetItemState(label, vtkKWOptions::StateNormal); 
};


//----------------------------------------------------------------------------
void vtkKWMenu::SetState(int state)
{
  int nb_of_items = this->GetNumberOfItems();
  if (!nb_of_items)
    {
    return;
    }

  ostrstream tk_cmd;
  const char *wname = this->GetWidgetName();

  const char *statestr = vtkKWOptions::GetStateAsTkOptionValue(state);

  for (int i = 0; i < nb_of_items; i++)
    {
    tk_cmd << "catch {" << wname << " entryconfigure " << i 
           << " -state " << statestr << "}" << endl;
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemImage(int index, const char *imgname)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
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
               this->GetWidgetName(), index, imgname);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemImageToPredefinedIcon(int index, int icon_index)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
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
  this->SetItemSelectImage(index, buffer);

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
  this->SetItemIndicatorVisibility(index, 0);
#endif

  this->SetItemImage(index, buffer);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectImage(int index, const char *imgname)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -selectimage %s", 
               this->GetWidgetName(), index, imgname);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemSelectImageToPredefinedIcon(int index, int icon_index)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
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
  this->SetItemSelectImage(index, buffer);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemCompoundMode(int index, int mode)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -compound %s", 
               this->GetWidgetName(), 
               index,
               vtkKWOptions::GetCompoundModeAsTkOptionValue(mode));
}

void vtkKWMenu::SetItemCompoundModeToNone(int index)     
{ 
  this->SetItemCompoundMode(index, vtkKWOptions::CompoundModeNone); 
};
void vtkKWMenu::SetItemCompoundModeToLeft(int index) 
{ 
  this->SetItemCompoundMode(index, vtkKWOptions::CompoundModeLeft); 
};
void vtkKWMenu::SetItemCompoundModeToCenter(int index) 
{ 
  this->SetItemCompoundMode(index, vtkKWOptions::CompoundModeCenter); 
};
void vtkKWMenu::SetItemCompoundModeToRight(int index) 
{ 
  this->SetItemCompoundMode(index, vtkKWOptions::CompoundModeRight); 
};
void vtkKWMenu::SetItemCompoundModeToTop(int index) 
{ 
  this->SetItemCompoundMode(index, vtkKWOptions::CompoundModeTop); 
};
void vtkKWMenu::SetItemCompoundModeToBottom(int index) 
{ 
  this->SetItemCompoundMode(index, vtkKWOptions::CompoundModeBottom);
};

//----------------------------------------------------------------------------
int vtkKWMenu::GetItemCompoundMode(int index)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return vtkKWOptions::CompoundModeUnknown;
    }

  return vtkKWOptions::GetCompoundModeFromTkOptionValue(
    this->GetItemOption(index, "-compound"));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemMarginVisibility(int index, int flag)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -hidemargin %d", 
               this->GetWidgetName(), index, flag ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemIndicatorVisibility(int index, int flag)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -indicatoron %d", 
               this->GetWidgetName(), index, flag ? 1 : 0);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemAccelerator(int index, const char *accelerator)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems() ||
      !this->HasItemOption(index, "-accelerator"))
    {
    return;
    }

  // Remove the old accelerator, if any

  if (!accelerator || !*accelerator)
    {
    const char *old_accelerator = this->GetItemOption(index, "-accelerator");
    if (old_accelerator && *old_accelerator)
      {
      char *keybinding = NULL;
      this->ConvertItemAcceleratorToKeyBinding(old_accelerator, &keybinding);
      if (keybinding && *keybinding)
        {
        vtkKWTopLevel *toplevel = this->GetParentTopLevel();
        if (toplevel)
          {
          toplevel->SetBinding(keybinding, NULL);
          }
        }
      delete [] keybinding;
      }
    }

  // Install the new one

  this->Script("%s entryconfigure %d -accelerator {%s}", 
               this->GetWidgetName(), index, accelerator ? accelerator : "");

  this->InstallItemAcceleratorBindingOnToplevel(index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::InstallItemAcceleratorBindingOnToplevel(int index)
{
  const char *accelerator = this->GetItemOption(index, "-accelerator");
  if (accelerator && *accelerator)
    {
    char *keybinding = NULL;
    this->ConvertItemAcceleratorToKeyBinding(accelerator, &keybinding);
    if (keybinding && *keybinding)
      {
      const char *command = this->GetItemCommand(index);
      if (command && *command)
        {
        vtksys_stl::string command_safe(command);
        vtkKWTopLevel *toplevel = this->GetParentTopLevel();
        if (toplevel)
          {
          toplevel->SetBinding(keybinding, command_safe.c_str());
          }
        }
      }
    delete [] keybinding;
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::ConvertItemAcceleratorToKeyBinding(
  const char *accelerator, char **keybinding)
{
  vtksys_stl::string keybinding_str;
  
  if (accelerator && *accelerator)
    {
    vtksys_stl::string accelerator_safe(accelerator);

    // Let's try to transform it into a binding

    vtksys::SystemTools::ReplaceString(
      accelerator_safe, "+", "-");
    vtksys::SystemTools::ReplaceString(
      accelerator_safe, "Ctrl", "Control");

    // Accelerator are usually specified as Ctrl-O, with the letter
    // in uppercase. Switch it to lowercase

    vtksys_stl::string::size_type accel_size = accelerator_safe.size();
    vtksys_stl::string::size_type last_sep = accelerator_safe.rfind("-");
    if((last_sep != vtksys_stl::string::npos &&
        last_sep == accel_size - 2) ||
       (last_sep == vtksys_stl::string::npos && accel_size == 1))
      {
      accelerator_safe[accel_size - 1] =
        static_cast<vtksys_stl::string::value_type>(
          tolower(accelerator_safe[accel_size - 1]));
      }

    keybinding_str = "<";
    if(last_sep == vtksys_stl::string::npos)
      {
      keybinding_str += "Key-";
      }
    keybinding_str += accelerator_safe;
    keybinding_str += ">";
    }

  *keybinding = new char[keybinding_str.size() + 1];
  strcpy(*keybinding, keybinding_str.c_str());
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemHelpString(int index, const char *help)
{
  vtksys_stl::string help_safe(help ? help : "");

  const char *label = this->GetItemLabel(index);
  if (!label || !*label)
    {
    return;
    }

  vtksys_stl::string label_safe(label);
  this->Script("set {%sHelpArray(%s)} {%s}", 
               this->GetTclName(), label_safe.c_str(), help_safe.c_str());
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemHelpString(int index)
{
  const char *label = this->GetItemLabel(index);
  if (!label || !*label)
    {
    return NULL;
    }

  // This hack should be cleaned

  vtksys_stl::string label_safe(label);
  const char *tname = this->GetTclName();
  return this->Script(
    "if [catch {set %sTemp $%sHelpArray(%s)} %sTemp ]"
    " { set %sTemp \"\"}; set %sTemp", 
    tname, tname, label_safe.c_str(), tname, tname, tname);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemUnderline(int index, int underline_index)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems() 
      || underline_index < 0)
    {
    return;
    }
  this->Script("%s entryconfigure %d -underline %d", 
               this->GetWidgetName(), index, underline_index);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetItemColumnBreak(int index, int flag)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return;
    }
  this->Script("%s entryconfigure %d -columnbreak %d", 
               this->GetWidgetName(), index, flag);
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
int vtkKWMenu::HasItemOption(int index, const char *option)
{
  if (!this->IsCreated() || index < 0 || index >= this->GetNumberOfItems())
    {
    return 0;
    }
 
  return !this->GetApplication()->EvaluateBooleanExpression(
    "catch {%s entrycget %d %s}",
    this->GetWidgetName(), index, option);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetItemOption(int index, const char *option)
{
  if (!this->HasItemOption(index, option))
    {
    return NULL;
    }
  return this->Script("%s entrycget %d %s", 
                      this->GetWidgetName(), index, option);
}

//----------------------------------------------------------------------------
void vtkKWMenu::DisplayHelpCallback(const char* widget_name)
{
  int index = this->GetIndexOfActiveItem(widget_name);
  if (index >= 0)
    {
    this->Internals->IndexOfLastActiveItem = index;
    }
  const char *help = this->GetItemHelpString(index);
  if(help)
    {
    vtksys_stl::string help_safe(help);
    vtkKWWindowBase *window = vtkKWWindowBase::SafeDownCast(
      this->GetParentTopLevel());
    if (window)
      {
      window->SetStatusText(help_safe.c_str());
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMenu::GetBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMenu::GetBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-background");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-background", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMenu::GetForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMenu::GetForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-foreground");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-foreground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMenu::GetActiveBackgroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMenu::GetActiveBackgroundColor()
{
  return this->GetConfigurationOptionAsColor("-activebackground");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetActiveBackgroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activebackground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMenu::GetActiveForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMenu::GetActiveForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-activeforeground");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetActiveForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-activeforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetBorderWidth(int width)
{
  this->SetConfigurationOptionAsInt("-bd", width);
}

//----------------------------------------------------------------------------
int vtkKWMenu::GetBorderWidth()
{
  return this->GetConfigurationOptionAsInt("-bd");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetRelief(int relief)
{
  this->SetConfigurationOption(
    "-relief", vtkKWOptions::GetReliefAsTkOptionValue(relief));
}

void vtkKWMenu::SetReliefToRaised()     
{ 
  this->SetRelief(vtkKWOptions::ReliefRaised); 
};
void vtkKWMenu::SetReliefToSunken() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSunken); 
};
void vtkKWMenu::SetReliefToFlat() 
{ 
  this->SetRelief(vtkKWOptions::ReliefFlat); 
};
void vtkKWMenu::SetReliefToRidge() 
{ 
  this->SetRelief(vtkKWOptions::ReliefRidge); 
};
void vtkKWMenu::SetReliefToSolid() 
{ 
  this->SetRelief(vtkKWOptions::ReliefSolid); 
};
void vtkKWMenu::SetReliefToGroove() 
{ 
  this->SetRelief(vtkKWOptions::ReliefGroove); 
};

//----------------------------------------------------------------------------
int vtkKWMenu::GetRelief()
{
  return vtkKWOptions::GetReliefFromTkOptionValue(
    this->GetConfigurationOption("-relief"));
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetFont(const char *font)
{
  this->SetConfigurationOption("-font", font);
}

//----------------------------------------------------------------------------
const char* vtkKWMenu::GetFont()
{
  return this->GetConfigurationOption("-font");
}

//----------------------------------------------------------------------------
void vtkKWMenu::GetDisabledForegroundColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMenu::GetDisabledForegroundColor()
{
  return this->GetConfigurationOptionAsColor("-disabledforeground");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetDisabledForegroundColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-disabledforeground", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMenu::GetSelectColor(double *r, double *g, double *b)
{
  this->GetConfigurationOptionAsColor("-selectcolor", r, g, b);
}

//----------------------------------------------------------------------------
double* vtkKWMenu::GetSelectColor()
{
  return this->GetConfigurationOptionAsColor("-selectcolor");
}

//----------------------------------------------------------------------------
void vtkKWMenu::SetSelectColor(double r, double g, double b)
{
  this->SetConfigurationOptionAsColor("-selectcolor", r, g, b);
}

//----------------------------------------------------------------------------
void vtkKWMenu::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "TearOff: " << this->GetTearOff() << endl;
}

