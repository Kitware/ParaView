/*=========================================================================

  Module:    vtkKWToolbarSet.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWToolbarSet - a "set of toolbars" widget
// .SECTION Description
// A simple widget representing a set of toolbars..

#ifndef __vtkKWToolbarSet_h
#define __vtkKWToolbarSet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWToolbar;
class vtkKWToolbarSetInternals;
class vtkKWMenu;

class KWWIDGETS_EXPORT vtkKWToolbarSet : public vtkKWWidget
{
public:
  static vtkKWToolbarSet* New();
  vtkTypeRevisionMacro(vtkKWToolbarSet,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget (a frame holding all the toolbars).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Get the frame that can be used as a parent to a toolbar
  vtkGetObjectMacro(ToolbarsFrame, vtkKWFrame);

  // Description:
  // Add a toolbar to the set.
  // The default_visibility parameter sets the visibility of the toolbar
  // in the set once it is added.
  // Return 1 on success, 0 otherwise.
  virtual int AddToolbar(vtkKWToolbar *toolbar, int default_visibility = 1);
  virtual int HasToolbar(vtkKWToolbar *toolbar);
  
  // Description:
  // Return a toolbar at a particular index.
  virtual vtkKWToolbar* GetToolbar(int index);
  virtual int GetNumberOfToolbars();

  // Description:
  // Set/Get the flat aspect of the toolbars
  virtual void SetToolbarsFlatAspect(int);

  // Description:
  // Set/Get the flat aspect of the widgets (flat or 3D GUI style)
  virtual void SetToolbarsWidgetsFlatAspect(int);

  // Description:
  // Convenience method to hide/show a toolbar
  virtual void HideToolbar(vtkKWToolbar *toolbar);
  virtual void ShowToolbar(vtkKWToolbar *toolbar);
  virtual void SetToolbarVisibility(vtkKWToolbar *toolbar, int flag);
  virtual int GetToolbarVisibility(vtkKWToolbar *toolbar);
  virtual void ToggleToolbarVisibility(vtkKWToolbar *toolbar);

  // Description:
  // Return the number of visible toolbars
  virtual int GetNumberOfVisibleToolbars();

  // Description:
  // Save/Restore the visibility flag of one/all toolbars to/from the registry
  // Note that the name of each toolbar to save/restore should have been set
  // for this method to work (see vtkKWToolbar).
  virtual void SaveToolbarVisibilityToRegistry(vtkKWToolbar *toolbar);
  virtual void RestoreToolbarVisibilityFromRegistry(vtkKWToolbar *toolbar);
  virtual void SaveToolbarsVisibilityToRegistry();
  virtual void RestoreToolbarsVisibilityFromRegistry();

  // Description:
  // Set/Get if the visibility flag of the toolbars should be saved
  // or restored to the registry automatically.
  // It is restored when the toolbar is added, and saved when the visibility
  // flag is changed.
  vtkBooleanMacro(SynchronizeToolbarsVisibilityWithRegistry, int); 
  vtkGetMacro(SynchronizeToolbarsVisibilityWithRegistry, int); 
  vtkSetMacro(SynchronizeToolbarsVisibilityWithRegistry, int); 

  // Description:
  // Convenience method to create and update a menu that can be used to control
  // the visibility of all toolbars.
  // The Populate...() method will repopulate the menu (note that it does 
  // *not* remove all entries, so that this menu can be used for several
  // toolbar sets).
  // The Update...() method will update the state of the entries according
  // to the toolbarsvisibility (the first one will call the second one
  // automatically).
  virtual void PopulateToolbarsVisibilityMenu(vtkKWMenu *menu);
  virtual void UpdateToolbarsVisibilityMenu(vtkKWMenu *menu);

  // Description:
  // Set/Get the command/callback that will be called when the visibility
  // of a toolbar is changed.
  virtual void SetToolbarVisibilityChangedCommand(
    vtkKWObject* object,const char *method);
  virtual void InvokeToolbarVisibilityChangedCommand();

  // Description:
  // Set/Get the command/callback that will be called when the number of
  // toolbar has changed (added or removed).
  virtual void SetNumberOfToolbarsChangedCommand(
    vtkKWObject* object,const char *method);
  virtual void InvokeNumberOfToolbarsChangedCommand();

  // Description:
  // Show or hide a separator at the bottom of the set
  virtual void SetShowBottomSeparator(int);
  vtkBooleanMacro(ShowBottomSeparator, int); 
  vtkGetMacro(ShowBottomSeparator, int); 

  // Description:
  // Update the toolbar set 
  // (update the enabled state of all toolbars, call PackToolbars(), etc.).
  virtual void Update();

  // Description:
  // (Re)Pack the toolbars, if needed (if the widget is created, and the
  // toolbar is created, AddToolbar will pack the toolbar automatically).
  virtual void PackToolbars();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWToolbarSet();
  ~vtkKWToolbarSet();

  vtkKWFrame *ToolbarsFrame;
  vtkKWFrame *BottomSeparatorFrame;

  int ShowBottomSeparator;
  int SynchronizeToolbarsVisibilityWithRegistry;

  char *ToolbarVisibilityChangedCommand;
  char *NumberOfToolbarsChangedCommand;

  //BTX

  // A toolbar slot stores a toolbar + some infos
 
  class ToolbarSlot
  {
  public:
    int Visibility;
    vtkKWFrame   *SeparatorFrame;
    vtkKWToolbar *Toolbar;
  };

  // PIMPL Encapsulation for STL containers

  vtkKWToolbarSetInternals *Internals;
  friend class vtkKWToolbarSetInternals;

  // Helper methods

  ToolbarSlot* GetToolbarSlot(vtkKWToolbar *toolbar);

  //ETX

  virtual void Pack();
  virtual void PackBottomSeparator();

private:
  vtkKWToolbarSet(const vtkKWToolbarSet&); // Not implemented
  void operator=(const vtkKWToolbarSet&); // Not implemented
};

#endif

