/*=========================================================================

  Module:    vtkKWMostRecentFilesUtilities.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkKWMostRecentFilesUtilities.h"

#include "vtkKWApplication.h"
#include "vtkObjectFactory.h"
#include "vtkKWMenu.h"

#include <vtkstd/string>
#include <vtkstd/list>

#include <kwsys/SystemTools.hxx>

vtkCxxRevisionMacro(vtkKWMostRecentFilesUtilities, "1.3");
vtkStandardNewMacro(vtkKWMostRecentFilesUtilities );

int vtkKWMostRecentFilesUtilitiesCommand(ClientData cd, Tcl_Interp *interp,
                                         int argc, char *argv[]);

#define VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN "File%d"
#define VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN "File%dCmd"
#define VTK_KW_MRF_REGISTRY_MAX_ENTRIES 50

//----------------------------------------------------------------------------
class vtkKWMostRecentFilesUtilitiesInternals
{
public:

  class MostRecentFileEntry
  {
  public:
    MostRecentFileEntry() { this->TargetObject = NULL; };
    ~MostRecentFileEntry() {};
    
    const char *GetFileName() 
      { return this->FileName.c_str(); }
    void SetFileName(const char *filename) 
      { this->FileName = filename; }
    
    const char *GetTargetCommand() 
      { return this->TargetCommand.c_str(); }
    void SetTargetCommand(const char *command) 
      { this->TargetCommand = command; }

    vtkKWObject *GetTargetObject() 
      { return this->TargetObject; }
    void SetTargetObject(vtkKWObject *object) 
      { this->TargetObject = object; }

    int IsEqual(const char *filename, vtkKWObject *, const char *) 
      { return (filename && !strcmp(filename, this->FileName.c_str())); }
  
  private:
    
    vtkstd::string FileName;
    vtkKWObject *TargetObject;
    vtkstd::string TargetCommand;
  };

  typedef vtkstd::list<vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntry*> MostRecentFileEntriesContainer;
  typedef vtkstd::list<vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntry*>::iterator MostRecentFileEntriesContainerIterator;

  MostRecentFileEntriesContainer MostRecentFileEntries;
};

//----------------------------------------------------------------------------
vtkKWMostRecentFilesUtilities::vtkKWMostRecentFilesUtilities()
{
  this->DefaultTargetObject   = NULL;
  this->DefaultTargetCommand  = NULL;
  this->RegistryKey           = NULL;
  this->MostRecentFilesMenu   = NULL;

  this->SetRegistryKey("MRU");

  this->MaximumNumberOfMostRecentFilesInRegistry = 10;
  this->MaximumNumberOfMostRecentFilesInMenu = 10;

  this->Internals = new vtkKWMostRecentFilesUtilitiesInternals;
}

//----------------------------------------------------------------------------
vtkKWMostRecentFilesUtilities::~vtkKWMostRecentFilesUtilities()
{
  this->SetDefaultTargetCommand(NULL);
  this->SetRegistryKey(NULL);

  if (this->MostRecentFilesMenu)
    {
    this->MostRecentFilesMenu->Delete();
    this->MostRecentFilesMenu = NULL;
    }

  if (this->Internals)
    {
    vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator it = this->Internals->MostRecentFileEntries.begin();
    vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator end = this->Internals->MostRecentFileEntries.end();
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
void vtkKWMostRecentFilesUtilities::AddMostRecentFileInternal(
  const char *filename, 
  vtkKWObject *target_object, 
  const char *target_command)
{
  if (!filename || !*filename)
    {
    return;
    }

  // Find if already inserted (and delete it)

  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator it = this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator end = this->Internals->MostRecentFileEntries.end();
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

  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntry
    *entry = new vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntry;
  entry->SetFileName(filename);
  entry->SetTargetObject(target_object);
  entry->SetTargetCommand(target_command);

  this->Internals->MostRecentFileEntries.push_front(entry);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::AddMostRecentFile(
  const char *filename, 
  vtkKWObject *target_object, 
  const char *target_command)
{
  if (!this->GetApplication())
    {
    vtkErrorMacro("Error! Application not set!");
    return;
    }

  vtkstd::string filename_expanded = 
    this->GetApplication()->ExpandFileName(filename);

  this->AddMostRecentFileInternal(
    filename_expanded.c_str(), target_object, target_command);

  this->UpdateMostRecentFilesMenu();
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::SetDefaultTargetObject(vtkKWObject *_arg)
{
  if (this->DefaultTargetObject == _arg) 
    { 
    return;
    }

  this->DefaultTargetObject = _arg;

  this->Modified();

  this->UpdateMostRecentFilesMenu();
} 

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::SetDefaultTargetCommand(const char* _arg)
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

  this->UpdateMostRecentFilesMenu();
} 

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::SaveMostRecentFilesToRegistry()
{
  this->SaveMostRecentFilesToRegistry(
    this->RegistryKey, 
    this->MaximumNumberOfMostRecentFilesInRegistry);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::SaveMostRecentFilesToRegistry(
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

  char filename_key[20], command_key[20];

  // Store all most recent files entries to registry

  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator it = this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator end = this->Internals->MostRecentFileEntries.end();
  int count = 0;
  for (; it != end && count < max_nb; ++it)
    {
    if (*it)
      {
      sprintf(filename_key,VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN,count);
      sprintf(command_key,VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN,count);

      const char* target_command = (*it)->GetTargetCommand();
      if (!target_command || !*target_command)
        {
        target_command = this->DefaultTargetCommand;
        }
      if (target_command && *target_command)
        {
        this->GetApplication()->SetRegistryValue(
          1, reg_key, filename_key, (*it)->GetFileName());
        this->GetApplication()->SetRegistryValue(
          1, reg_key, command_key, target_command);
        ++count;
        }
      }
    }
  
  // As a convenience, remove all others
  
  for (; count < VTK_KW_MRF_REGISTRY_MAX_ENTRIES; count++)
    {
    sprintf(filename_key,VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN,count);
    sprintf(command_key,VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN,count);
    this->GetApplication()->DeleteRegistryValue(
      1, reg_key, filename_key);
    this->GetApplication()->DeleteRegistryValue(
      1, reg_key, command_key);
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::LoadMostRecentFilesFromRegistry()
{
  this->LoadMostRecentFilesFromRegistry(
    this->RegistryKey, 
    this->MaximumNumberOfMostRecentFilesInRegistry);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::LoadMostRecentFilesFromRegistry(
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

  char filename_key[20], command_key[20], filename[1024], command[1024];

  int i;
  for (i = VTK_KW_MRF_REGISTRY_MAX_ENTRIES - 1; 
       i >= 0 && max_nb; 
       i--)
    {
    sprintf(filename_key, VTK_KW_MRF_REGISTRY_FILENAME_KEYNAME_PATTERN, i);
    sprintf(command_key, VTK_KW_MRF_REGISTRY_COMMAND_KEYNAME_PATTERN, i);
    if (this->GetApplication()->GetRegistryValue(
          1, reg_key, filename_key, filename) &&
        this->GetApplication()->GetRegistryValue(
          1, reg_key, command_key, command) &&
        strlen(filename) >= 1)
      {
      this->AddMostRecentFileInternal(filename, NULL, command);
      max_nb--;
      }
    }

  this->UpdateMostRecentFilesMenu();
}


//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::SetMaximumNumberOfMostRecentFilesInMenu(int _arg)
{
  if (this->MaximumNumberOfMostRecentFilesInMenu == _arg || _arg < 0)
    {
    return;
    }

  this->MaximumNumberOfMostRecentFilesInMenu = _arg;
  this->Modified();

  this->UpdateMostRecentFilesMenu();
}

//----------------------------------------------------------------------------
vtkKWMenu* vtkKWMostRecentFilesUtilities::GetMostRecentFilesMenu()
{ 
  if (!this->MostRecentFilesMenu)
    {
    this->MostRecentFilesMenu = vtkKWMenu::New();
    }
  return this->MostRecentFilesMenu;
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::UpdateMostRecentFilesMenu()
{ 
  this->PopulateMostRecentFilesMenu(
    this->MostRecentFilesMenu, this->MaximumNumberOfMostRecentFilesInMenu);
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::PopulateMostRecentFilesMenu(
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

  menu->DeleteAllMenuItems();

  // Fill the menu
  
  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator it = this->Internals->MostRecentFileEntries.begin();
  vtkKWMostRecentFilesUtilitiesInternals::MostRecentFileEntriesContainerIterator end = this->Internals->MostRecentFileEntries.end();
  int count = 0;
  for (; it != end && count < max_nb; ++it)
    {
    if (*it)
      {
      const char *filename = (*it)->GetFileName();
      vtkKWObject *target_object = (*it)->GetTargetObject();
      if (!target_object)
        {
        target_object= this->DefaultTargetObject;
        }
      const char *target_command = (*it)->GetTargetCommand();
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
        kwsys_stl::string short_file = 
          kwsys::SystemTools::CropString(filename, 40);
        ostrstream label;
        ostrstream cmd;
        label << count << " " << short_file.c_str() << ends;
        cmd << target_command << " {" << filename << "}" << ends;
        menu->AddCommand(
          label.str(), target_object, cmd.str(), (count < 10 ? 0 : -1),
          filename);
        label.rdbuf()->freeze(0);
        cmd.rdbuf()->freeze(0);
        count++;
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::UpdateMostRecentFilesMenuStateInParent()
{
  if (this->MostRecentFilesMenu && this->MostRecentFilesMenu->IsCreated())
    {
    vtkKWMenu *parent = 
      vtkKWMenu::SafeDownCast(this->MostRecentFilesMenu->GetParent());
    if (parent)
      {
      int index = parent->GetCascadeIndex(this->MostRecentFilesMenu);
      if (index >= 0)
        {
        int nb_items = this->MostRecentFilesMenu->GetNumberOfItems();
        int menu_enabled = 
          parent->GetEnabled() ? vtkKWMenu::Normal : vtkKWMenu::Disabled;
        parent->SetState(
          index,  nb_items ? menu_enabled : vtkKWMenu::Disabled);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWMostRecentFilesUtilities::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "MaximumNumberOfMostRecentFilesInRegistry: " 
     << this->MaximumNumberOfMostRecentFilesInRegistry << endl;
  os << indent << "MaximumNumberOfMostRecentFilesInMenu: " 
     << this->MaximumNumberOfMostRecentFilesInMenu << endl;
  os << indent << "DefaultTargetObject: " 
     << this->DefaultTargetObject << endl;
  os << indent << "DefaultTargetCommand: " 
     << (this->DefaultTargetCommand ? this->DefaultTargetCommand : "NULL") 
     << endl;
  os << indent << "RegistryKey: " 
     << (this->RegistryKey ? this->RegistryKey : "NULL") 
     << endl;
}


