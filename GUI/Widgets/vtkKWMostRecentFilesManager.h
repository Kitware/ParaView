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
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWWindowBase

#ifndef __vtkKWMostRecentFilesManager_h
#define __vtkKWMostRecentFilesManager_h

#include "vtkKWObject.h"

class vtkKWMostRecentFilesManagerInternals;
class vtkKWMenu;

class KWWidgets_EXPORT vtkKWMostRecentFilesManager : public vtkKWObject
{
public:
  static vtkKWMostRecentFilesManager* New();
  vtkTypeRevisionMacro(vtkKWMostRecentFilesManager,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a most recent file to the list.
  // Each most recent file is associated to a target object and a target 
  // command: when a most recent file is invoked (either programmatically or
  // using the most recent files menu), the associated target commmand is 
  // invoked on the associated target object. If one and/or the other was not
  // specified when the most recent file was added, the default target
  // object and/or default target command are used.
  // An optional label can be specified too.
  // This label will be used in the menu if LabelVisibilityInMenu is true.
  // The entry is always added at the beginning of the list (thus, becoming
  // the most recently used).
  virtual void AddFile(
    const char *filename, 
    vtkObject *target_object = NULL, 
    const char *target_command = NULL,
    const char *label = NULL);
  
  // Description:
  // Set/Get the default target object and command.
  // Each most recent file is associated to a target object and a target 
  // command: when a most recent file is invoked (either programmatically or
  // using the most recent files menu), the associated target commmand is 
  // invoked on the associated target object. If one and/or the other was not
  // specified when the most recent file was added, the default target
  // object and default target command are used.
  vtkGetObjectMacro(DefaultTargetObject, vtkObject);
  virtual void SetDefaultTargetObject(vtkObject *object);
  vtkGetStringMacro(DefaultTargetCommand);
  virtual void SetDefaultTargetCommand(const char *);

  // Description:
  // Load/Save up to 'max_nb' most recent files from/to the registry under
  // the application's 'reg_key' key.
  // The parameter-less methods use RegistryKey as 'reg_key' and
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
  // Get a most recent files menu object. 
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
  // Set a label for a specific file entry.
  // This label will be used in the menu if LabelVisibilityInMenu is true.
  virtual void SetFileLabel(const char *filename, const char *label);

  // Description:
  // Set/Get the label visibility in menu (Off by default).
  virtual void SetLabelVisibilityInMenu(int);
  vtkBooleanMacro(LabelVisibilityInMenu, int);
  vtkGetMacro(LabelVisibilityInMenu, int);

  // Description:
  // Separate path from basename in in menu (Off by default).
  virtual void SetSeparatePathInMenu(int);
  vtkBooleanMacro(SeparatePathInMenu, int);
  vtkGetMacro(SeparatePathInMenu, int);

  // Description:
  // Disable/Enable the most recent files menu if it is a cascade in its
  // parent. If the menu is empty, it will be disabled.
  virtual void UpdateMenuStateInParent();

  // Description:
  // Populate a given menu with up to 'max_nb' most recent files entries.
  // You do not need to call this method on GetMenu(), the
  // internal menu is updated automatically.
  virtual void PopulateMenu(vtkKWMenu*, int max_nb);

protected:
  vtkKWMostRecentFilesManager();
  ~vtkKWMostRecentFilesManager();

  char        *DefaultTargetCommand;
  vtkObject   *DefaultTargetObject;
  char        *RegistryKey;
  int         MaximumNumberOfFilesInRegistry;
  int         MaximumNumberOfFilesInMenu;
  int         LabelVisibilityInMenu;
  int         SeparatePathInMenu;

  //BTX

  // PIMPL Encapsulation for STL containers

  vtkKWMostRecentFilesManagerInternals *Internals;
  friend class vtkKWMostRecentFilesManagerInternals;

  //ETX

  // Description:
  // Add a most recent file to the list (internal, do not update the menu
  // or save to the registry).
  virtual void AddFileInternal(
    const char *filename, 
    vtkObject *target_object = NULL, 
    const char *target_command = NULL,
    const char *label = NULL);

  // Description:
  // Update the most recent files menu
  virtual void UpdateMenu();

private:
  
  // In private for lazy allocation using GetMenu()

  vtkKWMenu *Menu;

  vtkKWMostRecentFilesManager(const vtkKWMostRecentFilesManager&); // Not implemented
  void operator=(const vtkKWMostRecentFilesManager&); // Not implemented
};

#endif

