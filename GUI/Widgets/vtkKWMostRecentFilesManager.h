/*=========================================================================

  Module:    vtkKWMostRecentFilesManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMostRecentFilesManager - a "most recent files" manager.
// .SECTION Description
// This class is basically a manager that acts as a container for a set of
// most recent files.
// It provides methods to manipulate them, load/save them from/to the
// registry, and display them as entries in a menu.
// An instance of this class is created in vtkKWWindowBase
// .SECTION See Also
// vtkKWWindowBase

#ifndef __vtkKWMostRecentFilesManager_h
#define __vtkKWMostRecentFilesManager_h

#include "vtkKWObject.h"

class vtkKWMostRecentFilesManagerInternals;
class vtkKWMenu;

class VTK_EXPORT vtkKWMostRecentFilesManager : public vtkKWObject
{
public:
  static vtkKWMostRecentFilesManager* New();
  vtkTypeRevisionMacro(vtkKWMostRecentFilesManager,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description::
  // Add a most recent file to the list.
  // Each most recent file is associated to a target object and a target 
  // command: when a most recent file is invoked (either programmatically or
  // using the most recent files menu), the associated target commmand is 
  // invoked on the associated target object. If one and/or the other was not
  // specified when the most recent file was added, the default target
  // object and/or default target command are used.
  virtual void AddFile(
    const char *filename, 
    vtkKWObject *target_object = NULL, 
    const char *target_command = NULL);
  
  // Description:
  // Set/Get the default target object and command.
  // Each most recent file is associated to a target object and a target 
  // command: when a most recent file is invoked (either programmatically or
  // using the most recent files menu), the associated target commmand is 
  // invoked on the associated target object. If one and/or the other was not
  // specified when the most recent file was added, the default target
  // object and default target command are used.
  vtkGetObjectMacro(DefaultTargetObject, vtkKWObject);
  virtual void SetDefaultTargetObject(vtkKWObject*);
  vtkGetStringMacro(DefaultTargetCommand);
  virtual void SetDefaultTargetCommand(const char *);

  // Description:
  // Load/Save up to 'max_nb' most recent files from/to the registry under
  // the application's 'reg_key' key.
  // The parameter-less methods use DefaultRegistryKey as 'reg_key' and
  // MaximumNumberOfFilesInRegistry as 'max_nb'.
  // Only the filename and target command are saved. When entries are loaded
  // make sure DefaultTargetObject is set to a valid object.
  virtual void RestoreFilesListFromRegistry();
  virtual void SaveFilesToRegistry();
  virtual void RestoreFilesListFromRegistry(
    const char *reg_key, int max_nb);
  virtual void SaveFilesToRegistry(
    const char *reg_key, int max_nb);

  // Description:
  // Set/Get the default registry key the most recent files are saved to or
  // loaded from when it is not specified explicitly while calling load/save.
  vtkGetStringMacro(RegistryKey);
  vtkSetStringMacro(RegistryKey);

  // Description:
  // Set/Get the default maximum number of recent files in the registry when
  // this number is not specified explicitly while calling load/save.
  vtkGetMacro(MaximumNumberOfFilesInRegistry, int);
  vtkSetMacro(MaximumNumberOfFilesInRegistry, int);

  // Description:
  // Get a convenience most recent files menu object. 
  // This menu is updated automatically as entries are added and removed,
  // up to MaximumNumberOfFilesInMenu entries.
  // It is up to the caller to set its parent, and it should be done as soon as
  // possible if there is a need to use this menu.
  vtkKWMenu* GetMenu();

  // Description:
  // Set/Get the maximum number of recent files in the internal most recent
  // files menu object (see GetMenu()).
  vtkGetMacro(MaximumNumberOfFilesInMenu, int);
  virtual void SetMaximumNumberOfFilesInMenu(int);

  // Description:
  // Disable/Enable the most recent files menu if it is a cascade in its
  // parent. If the menu is empty, it will be disabled.
  virtual void UpdateMenuStateInParent();

  // Description:
  // Convenience method to populate a given menu with up to 'max_nb' 
  // most recent files entries.
  // You do not need to call this method on GetMenu(), the
  // internal menu is updated automatically.
  virtual void PopulateMenu(vtkKWMenu*, int max_nb);

protected:
  vtkKWMostRecentFilesManager();
  ~vtkKWMostRecentFilesManager();

  char        *DefaultTargetCommand;
  vtkKWObject *DefaultTargetObject;
  char        *RegistryKey;
  int         MaximumNumberOfFilesInRegistry;
  int         MaximumNumberOfFilesInMenu;

  //BTX

  // PIMPL Encapsulation for STL containers

  vtkKWMostRecentFilesManagerInternals *Internals;
  friend class vtkKWMostRecentFilesManagerInternals;

  //ETX

  // Description::
  // Add a most recent file to the list (internal, do not update the menu
  // or save to the registry).
  virtual void AddFileInternal(
    const char *filename, 
    vtkKWObject *target_object = NULL, 
    const char *target_command = NULL);

  // Description::
  // Update the most recent files menu
  virtual void UpdateMenu();

private:
  
  // In private for lazy allocation using GetMenu()

  vtkKWMenu *Menu;

  vtkKWMostRecentFilesManager(const vtkKWMostRecentFilesManager&); // Not implemented
  void operator=(const vtkKWMostRecentFilesManager&); // Not implemented
};

#endif

