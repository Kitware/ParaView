/*=========================================================================

  Module:    vtkKWMostRecentFilesUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMostRecentFilesUtilities - a set of most recent files
// .SECTION Description
// This class is basically a container for a set of most recent files.
// It provides methods to manipulate them, load/save them from/to the
// registry, and display them as entries in a menu.

#ifndef __vtkKWMostRecentFilesUtilities_h
#define __vtkKWMostRecentFilesUtilities_h

#include "vtkKWObject.h"

class vtkKWMostRecentFilesUtilitiesInternals;
class vtkKWMenu;

class VTK_EXPORT vtkKWMostRecentFilesUtilities : public vtkKWObject
{
public:
  static vtkKWMostRecentFilesUtilities* New();
  vtkTypeRevisionMacro(vtkKWMostRecentFilesUtilities,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

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

  // Description::
  // Add a most recent file to the list.
  // Each most recent file is associated to a target object and a target 
  // command: when a most recent file is invoked (either programmatically or
  // using the most recent files menu), the associated target commmand is 
  // invoked on the associated target object. If one and/or the other was not
  // specified when the most recent file was added, the default target
  // object and/or default target command are used.
  virtual void AddMostRecentFile(
    const char *filename, 
    vtkKWObject *target_object = NULL, 
    const char *target_command = NULL);
  
  // Description:
  // Load/Save up to 'max_nb' most recent files from/to the registry under
  // the application's 'reg_key' key.
  // The parameter-less methods use DefaultRegistryKey as 'reg_key' and
  // MaximumNumberOfMostRecentFilesInRegistry as 'max_nb'.
  // Only the filename and target command are saved. When entries are loaded
  // make sure DefaultTargetObject is set to a valid object.
  virtual void LoadMostRecentFilesFromRegistry();
  virtual void SaveMostRecentFilesToRegistry();
  virtual void LoadMostRecentFilesFromRegistry(
    const char *reg_key, int max_nb);
  virtual void SaveMostRecentFilesToRegistry(
    const char *reg_key, int max_nb);

  // Description:
  // Set/Get the default registry key the most recent files are saved to or
  // loaded from when it is not specified explicitly while calling load/save.
  vtkGetStringMacro(RegistryKey);
  vtkSetStringMacro(RegistryKey);

  // Description:
  // Set/Get the default maximum number of recent files in the registry when
  // this number is not specified explicitly while calling load/save.
  vtkGetMacro(MaximumNumberOfMostRecentFilesInRegistry, int);
  vtkSetMacro(MaximumNumberOfMostRecentFilesInRegistry, int);

  // Description:
  // Get the internal most recent files menu object. 
  // It is updated automatically as entries are added and removed,
  // up to MaximumNumberOfMostRecentFilesInMenu entries.
  // It is *not* up to this class to set its parent, do it as soon as
  // possible if you plan to use this menu.
  vtkKWMenu* GetMostRecentFilesMenu();

  // Description:
  // Set/Get the maximum number of recent files in the internal most recent
  // files menu object (see GetMostRecentFilesMenu()).
  vtkGetMacro(MaximumNumberOfMostRecentFilesInMenu, int);
  virtual void SetMaximumNumberOfMostRecentFilesInMenu(int);

  // Description:
  // Disable/Enable the most recent files menu if it is a cascade in its
  // parent. If the menu is empty, it will be disabled.
  virtual void UpdateMostRecentFilesMenuStateInParent();

  // Description:
  // Convenience method to populate a given menu with the up to 'max_nb' 
  // most recent files entries.
  // You do not need to call this method on GetMostRecentFilesMenu(), the
  // internal menu is updated automatically.
  virtual void PopulateMostRecentFilesMenu(vtkKWMenu*, int max_nb);

protected:
  vtkKWMostRecentFilesUtilities();
  ~vtkKWMostRecentFilesUtilities();

  char        *DefaultTargetCommand;
  vtkKWObject *DefaultTargetObject;
  char        *RegistryKey;
  int         MaximumNumberOfMostRecentFilesInRegistry;
  int         MaximumNumberOfMostRecentFilesInMenu;

  //BTX

  // PIMPL Encapsulation for STL containers

  vtkKWMostRecentFilesUtilitiesInternals *Internals;
  friend class vtkKWMostRecentFilesUtilitiesInternals;

  //ETX

  // Description::
  // Add a most recent file to the list (internal, do not update the menu
  // or save to the registry).
  virtual void AddMostRecentFileInternal(
    const char *filename, 
    vtkKWObject *target_object = NULL, 
    const char *target_command = NULL);

  // Description::
  // Update the most recent files menu
  virtual void UpdateMostRecentFilesMenu();

private:
  
  // In private for lazy allocation using GetMostRecentFilesMenu()

  vtkKWMenu   *MostRecentFilesMenu;

  vtkKWMostRecentFilesUtilities(const vtkKWMostRecentFilesUtilities&); // Not implemented
  void operator=(const vtkKWMostRecentFilesUtilities&); // Not implemented
};

#endif

