/*=========================================================================

  Module:    vtkKWMostRecentFilesManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMostRecentFilesManager.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWMenu.h"

#include <vtksys/stl/string>
#include <vtksys/stl/list>
#include <vtksys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkKWMostRecentFilesManager, "1.14");
vtkStandardNewMacro(vtkKWMostRecentFilesManager );

#define VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN "File%02d"
#define VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN "File%02dCmd"
#define VTK_KW_MRF_REGISTRY_LABEL_KEYNAME_PATTERN "File%02dLabel"

#define VTK_KW_MRF_REGISTRY_MAX_ENTRIES 50

//----------------------------------------------------------------------------
class vtkKWMostRecentFilesManagerInternals
{
public:

  class FileEntry
  {
  public:
    FileEntry() { this->TargetObject = NULL; };
    ~FileEntry() {};
    
    vtksys_stl::string FileName;
    vtkObject          *TargetObject;
    vtksys_stl::string TargetCommand;
    vtksys_stl::string Label;

    int IsEqual(const char *filename, vtkObject *, const char *) 
      { return (filename && !strcmp(filename, this->FileName.c_str())); }
  };

  typedef vtksys_stl::list<FileEntry*> FileEntriesContainer;
  typedef vtksys_stl::list<FileEntry*>::iterator FileEntriesContainerIterator;

  FileEntriesContainer MostRecentFileEntries;
};

//----------------------------------------------------------------------------
vtkKWMostRecentFilesManager::vtkKWMostRecentFilesManager()
{
  this->DefaultTargetObject   = NULL;
  this->DefaultTargetCommand  = NULL;
  this->RegistryKey           = NULL;
  this->Menu                  = NULL;
  this->LabelVisibilityInMenu = 0;
  this->SeparatePathInMenu    = 0;

  this->SetRegistryKey("MRU");

  this->MaximumNumberOfFilesInRegistry = 15;
  this->MaximumNumberOfFilesInMenu = 15;

  this->Internals = new vtkKWMostRecentFilesManagerInternals;
}

//----------------------------------------------------------------------------
vtkKWMostRecentFilesManager::~vtkKWMostRecentFilesManager()
{
  this->SetDefaultTargetCommand(NULL);
  this->SetRegistryKey(NULL);

  if (this->Menu)
    {
    this->Menu->Delete();
    this->Menu = NULL;
    }

  if (this->Internals)
    {
    vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator it = 
      this->Internals->MostRecentFileEntries.begin();
    vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator end = 
      this->Internals->MostRecentFileEntries.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        delete *it;
        }
      }
    delete this->Internals;
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::AddFileInternal(
  const char *filename, 
  vtkObject *target_object, 
  const char *target_command,
  const char *label)
{
  if (!filename || !*filename)
    {
    return;
    }

  // Find if already inserted (and delete it)

  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator it = 
    this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator end = 
    this->Internals->MostRecentFileEntries.end();
  for (; it != end; ++it)
    {
    if (*it && (*it)->IsEqual(filename, target_object, target_command))
      {
      delete *it;
      this->Internals->MostRecentFileEntries.erase(it);
      break;
      }
    }

  // Create new entry and prepend to container

  vtkKWMostRecentFilesManagerInternals::FileEntry
    *entry = new vtkKWMostRecentFilesManagerInternals::FileEntry;
  entry->FileName = filename;
  entry->TargetObject = target_object;
  entry->TargetCommand = target_command;
  if (label && *label)
    {
    entry->Label = label;
    }

  this->Internals->MostRecentFileEntries.push_front(entry);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::AddFile(
  const char *filename, 
  vtkObject *target_object, 
  const char *target_command,
  const char *label)
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  vtksys_stl::string evalstr = "eval file join {\"";
  evalstr += filename;
  evalstr += "\"}";

  vtksys_stl::string filename_expanded = 
    this->Script(evalstr.c_str());

  this->AddFileInternal(
    filename_expanded.c_str(), target_object, target_command, label);

  this->UpdateMenu();
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SetFileLabel(
  const char *filename, const char *label)
{
  if (!filename || !*filename)
    {
    return;
    }

  // Find entry and set label

  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator it = 
    this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator end = 
    this->Internals->MostRecentFileEntries.end();
  for (; it != end; ++it)
    {
    if (*it && !strcmp((*it)->FileName.c_str(), filename))
      {
      (*it)->Label = label ? label : "";
      this->UpdateMenu();
      break;
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SetDefaultTargetObject(vtkObject *_arg)
{
  if (this->DefaultTargetObject == _arg) 
    { 
    return;
    }

  this->DefaultTargetObject = _arg;

  this->Modified();

  this->UpdateMenu();
} 

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SetDefaultTargetCommand(const char* _arg)
{
  if (this->DefaultTargetCommand == NULL && _arg == NULL) 
    { 
    return;
    }
  if (this->DefaultTargetCommand && _arg && 
      (!strcmp(this->DefaultTargetCommand, _arg))) 
    { 
    return;
    }
  if (this->DefaultTargetCommand) 
    { 
    delete [] this->DefaultTargetCommand; 
    }
  if (_arg)
    {
    this->DefaultTargetCommand = new char[strlen(_arg) + 1];
    strcpy(this->DefaultTargetCommand, _arg);
    }
   else
    {
    this->DefaultTargetCommand = NULL;
    }

  this->Modified();

  this->UpdateMenu();
} 

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SaveFilesToRegistry()
{
  this->SaveFilesToRegistry(
    this->RegistryKey, 
    this->MaximumNumberOfFilesInRegistry);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SaveFilesToRegistry(
  const char *reg_key, int max_nb)
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  if (!reg_key)
    {
    vtkErrorMacro("Error! Can not save to empty key in registry!");
    return;
    }

  char filename_key[20], command_key[20], label_key[20];

  // Store all most recent files entries to registry

  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator it = 
    this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator end = 
    this->Internals->MostRecentFileEntries.end();
  int count = 0;
  for (; it != end && count < max_nb; ++it)
    {
    if (*it)
      {
      sprintf(filename_key,
              VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN, count);
      sprintf(command_key,
              VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN, count);
      sprintf(label_key,
              VTK_KW_MRF_REGISTRY_LABEL_KEYNAME_PATTERN, count);

      const char* target_command = (*it)->TargetCommand.c_str();
      if (!target_command || !*target_command)
        {
        target_command = this->DefaultTargetCommand;
        }
      if (target_command && *target_command)
        {
        this->GetApplication()->SetRegistryValue(
          1, reg_key, filename_key, (*it)->FileName.c_str());
        this->GetApplication()->SetRegistryValue(
          1, reg_key, command_key, target_command);
        if ((*it)->Label.size())
          {
          this->GetApplication()->SetRegistryValue(
            1, reg_key, label_key, (*it)->Label.c_str());
          }
        ++count;
        }
      }
    }
  
  // As a convenience, remove all others
  
  for (; count < VTK_KW_MRF_REGISTRY_MAX_ENTRIES; count++)
    {
    sprintf(filename_key,
            VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN, count);
    sprintf(command_key,
            VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN, count);
      sprintf(label_key,
              VTK_KW_MRF_REGISTRY_LABEL_KEYNAME_PATTERN, count);
    this->GetApplication()->DeleteRegistryValue(
      1, reg_key, filename_key);
    this->GetApplication()->DeleteRegistryValue(
      1, reg_key, command_key);
    this->GetApplication()->DeleteRegistryValue(
      1, reg_key, label_key);
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::RestoreFilesListFromRegistry()
{
  this->RestoreFilesListFromRegistry(
    this->RegistryKey, 
    this->MaximumNumberOfFilesInRegistry);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::RestoreFilesListFromRegistry(
  const char *reg_key, int max_nb)
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  if (!reg_key)
    {
    vtkErrorMacro("Error! Can not load from empty key in registry!");
    return;
    }

  char filename_key[20], command_key[20], label_key[20];
  char filename[1024], command[1024], label[1024];

  int i;
  for (i = VTK_KW_MRF_REGISTRY_MAX_ENTRIES - 1; 
       i >= 0 && max_nb; 
       i--)
    {
    sprintf(filename_key, VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN, i);
    sprintf(command_key, VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN, i);
    sprintf(label_key, VTK_KW_MRF_REGISTRY_LABEL_KEYNAME_PATTERN, i);
    if (this->GetApplication()->GetRegistryValue(
          1, reg_key, filename_key, filename) &&
        this->GetApplication()->GetRegistryValue(
          1, reg_key, command_key, command) &&
        strlen(filename) >= 1)
      {
      if (!this->GetApplication()->GetRegistryValue(
            1, reg_key, label_key, label))
        {
        *label = '\0';
        }
      this->AddFileInternal(filename, NULL, command, label);
      max_nb--;
      }
    }

  this->UpdateMenu();
}


//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SetMaximumNumberOfFilesInMenu(int _arg)
{
  if (this->MaximumNumberOfFilesInMenu == _arg || _arg < 0)
    {
    return;
    }

  this->MaximumNumberOfFilesInMenu = _arg;
  this->Modified();

  this->UpdateMenu();
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWMostRecentFilesManager::GetMenu()
{ 
  if (!this->Menu)
    {
    this->Menu = vtkKWMenu::New();
    }
  return this->Menu;
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::UpdateMenu()
{ 
  this->PopulateMenu(
    this->Menu, this->MaximumNumberOfFilesInMenu);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::PopulateMenu(
  vtkKWMenu *menu, int max_nb)
{ 
  if (!menu)
    {
    vtkErrorMacro("Error! Can not populate NULL menu!");
    return;
    }
  if (!menu->IsCreated())
    {
    vtkErrorMacro("Error! Can not populate menu that was not created!");
    return;
    }

  menu->DeleteAllItems();

  // Fill the menu
  
  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator it = 
    this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesManagerInternals::FileEntriesContainerIterator end = 
    this->Internals->MostRecentFileEntries.end();
  int count = 0;
  for (; it != end && count < max_nb; ++it)
    {
    if (*it)
      {
      const char *filename = (*it)->FileName.c_str();
      vtkObject *target_object = (*it)->TargetObject;
      if (!target_object)
        {
        target_object= this->DefaultTargetObject;
        }
      const char *target_command = (*it)->TargetCommand.c_str();
      if (!target_command || !*target_command)
        {
        target_command= this->DefaultTargetCommand;
        }

      if (filename && *filename && target_command && *target_command)
        {
        if (!target_object)
          {
          vtkErrorMacro("Error! Can not add entry with empty target object!");
          continue;
          }
        vtksys_stl::string cmd(target_command);
        cmd += " {";
        cmd += filename;
        cmd += "}";

        char buffer[10];
        sprintf(buffer, "%d", count);
        vtksys_stl::string label(buffer);
        label += " - ";

        int has_label = this->LabelVisibilityInMenu && (*it)->Label.size();
        if (has_label)
          {
          label += (*it)->Label;
          label += " ";
          }

        if (this->SeparatePathInMenu)
          {
          if (has_label)
            {
            label += "(";
            }
          label += vtksys::SystemTools::CropString(
            vtksys::SystemTools::GetFilenameName(filename), 40);
          label += " ";
          if (!has_label)
            {
            label += "(";
            }
          label += "in ";
          label += vtksys::SystemTools::CropString(
            vtksys::SystemTools::GetFilenamePath(filename), 40);
          label += ")";
          }
        else
          {
          if (has_label)
            {
            label += "(";
            }
          label += vtksys::SystemTools::CropString(filename, 40);
          if (has_label)
            {
            label += ")";
            }
          }

        int index = menu->AddCommand(
          label.c_str(), target_object, cmd.c_str());
        if (index >= 0)
          {
          menu->SetItemHelpString(index, filename);
          if (count < 10)
            {
            menu->SetItemUnderline(index, 0);
            }
          }
        count++;
        }
      }
    }
}

// ----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SetLabelVisibilityInMenu(int _arg)
{
  if (this->LabelVisibilityInMenu == _arg)
    {
    return;
    }
  this->LabelVisibilityInMenu = _arg;
  this->Modified();

  this->UpdateMenu();
}

// ----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::SetSeparatePathInMenu(int _arg)
{
  if (this->SeparatePathInMenu == _arg)
    {
    return;
    }
  this->SeparatePathInMenu = _arg;
  this->Modified();

  this->UpdateMenu();
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::UpdateMenuStateInParent()
{
  if (this->Menu && this->Menu->IsCreated())
    {
    vtkKWMenu *parent = 
      vtkKWMenu::SafeDownCast(this->Menu->GetParent());
    if (parent)
      {
      int index = parent->GetIndexOfCascadeItem(this->Menu);
      if (index >= 0)
        {
        int nb_items = this->Menu->GetNumberOfItems();
        int menu_enabled = 
          parent->GetEnabled() ? vtkKWTkOptions::StateNormal : vtkKWTkOptions::StateDisabled;
        parent->SetItemState(
          index,  nb_items ? menu_enabled : vtkKWTkOptions::StateDisabled);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MaximumNumberOfFilesInRegistry: " 
     << this->MaximumNumberOfFilesInRegistry << endl;
  os << indent << "MaximumNumberOfFilesInMenu: " 
     << this->MaximumNumberOfFilesInMenu << endl;
  os << indent << "DefaultTargetObject: " 
     << this->DefaultTargetObject << endl;
  os << indent << "DefaultTargetCommand: " 
     << (this->DefaultTargetCommand ? this->DefaultTargetCommand : "NULL") 
     << endl;
  os << indent << "RegistryKey: " 
     << (this->RegistryKey ? this->RegistryKey : "NULL") 
     << endl;
  os << indent << "LabelVisibilityInMenu: " 
     << (this->LabelVisibilityInMenu ? "On" : "Off") << endl;
  os << indent << "SeparatePathInMenu: " 
     << (this->SeparatePathInMenu ? "On" : "Off") << endl;
}


