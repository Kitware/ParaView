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

#include "vtkKWCompositeWidget.h"

class vtkKWFrame;
class vtkKWMenu;
class vtkKWSeparator;
class vtkKWToolbar;
class vtkKWToolbarSetInternals;

class KWWidgets_EXPORT vtkKWToolbarSet : public vtkKWCompositeWidget
{
public:
  static vtkKWToolbarSet* New();
  vtkTypeRevisionMacro(vtkKWToolbarSet,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Get the frame that can be used as a parent to a toolbar
  vtkGetObjectMacro(ToolbarsFrame, vtkKWFrame);

  // Description:
  // Add a toolbar to the set.
  // The default_visibility parameter sets the visibility of the toolbar
  // in the set once it is added (so that it can be added hidden for example,
  // before its visibility setting is retrieved from the registry).
  // Return 1 on success, 0 otherwise.
  virtual int AddToolbar(vtkKWToolbar *toolbar)
    { return this->AddToolbar(toolbar, 1); };
  virtual int AddToolbar(vtkKWToolbar *toolbar, int default_visibility);
  virtual int HasToolbar(vtkKWToolbar *toolbar);
  
  // Description:
  // Get the n-th toolbar, and the number of toolbars.
  virtual vtkKWToolbar* GetNthToolbar(int rank);
  virtual int GetNumberOfToolbars();

  // Description:
  // Remove a toolbar (or all) from the set.
  // Return 1 on success, 0 otherwise.
  virtual int RemoveToolbar(vtkKWToolbar *toolbar);
  virtual void RemoveAllToolbars();

  // Description:
  // Set/Get the flat aspect of the toolbars
  virtual void SetToolbarsFlatAspect(int);

  // Description:
  // Set/Get the flat aspect of the widgets (flat or 3D GUI style)
  virtual void SetToolbarsWidgetsFlatAspect(int);

  // Description:
  // Set the visibility of a toolbar.
  virtual void HideToolbar(vtkKWToolbar *toolbar);
  virtual void ShowToolbar(vtkKWToolbar *toolbar);
  virtual void SetToolbarVisibility(vtkKWToolbar *toolbar, int flag);
  virtual int GetToolbarVisibility(vtkKWToolbar *toolbar);
  virtual void ToggleToolbarVisibility(vtkKWToolbar *toolbar);

  // Description:
  // Return the number of visible toolbars
  virtual int GetNumberOfVisibleToolbars();

  // Description:
  // Set a toolbar's anchor. By default, toolbars are packed from left
  // to right in the order they were added to the toolbar set, i.e. each
  // toolbar is "anchored" to the west side of the set. One can change
  // this anchor on a per-toolbar basis. This means that all toolbars anchored
  // to the west side will be grouped together on that side, and all toolbars
  // anchored to the east side will be grouped on the opposite side. Note
  // though that anchoring acts like a "mirror": packing starts from the
  // anchor side, progressing towards the middle of the toolbar set (i.e.,
  // toolbars anchored west are packed left to right, toolbars anchored east
  // are packed right to left, following the order they were inserted in
  // the set).
  //BTX
  enum 
  {
    ToolbarAnchorWest = 0,
    ToolbarAnchorEast
  };
  //ETX
  virtual void SetToolbarAnchor(vtkKWToolbar *toolbar, int anchor);
  virtual int GetToolbarAnchor(vtkKWToolbar *toolbar);
  virtual void SetToolbarAnchorToWest(vtkKWToolbar *toolbar)
    { this->SetToolbarAnchor(toolbar, vtkKWToolbarSet::ToolbarAnchorWest); };
  virtual void SetToolbarAnchorToEast(vtkKWToolbar *toolbar)
    { this->SetToolbarAnchor(toolbar, vtkKWToolbarSet::ToolbarAnchorEast); };

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
  // Create and update a menu that can be used to control the visibility of
  // all toolbars.
  // The Populate...() method will repopulate the menu (note that it does 
  // *not* remove all entries, so that this menu can be used for several
  // toolbar sets).
  // The Update...() method will update the state of the entries according
  // to the toolbarsvisibility (the first one will call the second one
  // automatically).
  virtual void PopulateToolbarsVisibilityMenu(vtkKWMenu *menu);
  virtual void UpdateToolbarsVisibilityMenu(vtkKWMenu *menu);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the visibility of a toolbar is changed.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  // The following parameters are also passed to the command:
  // - pointer to the toolbar which visibility changed: vtkKWToolbar*
  virtual void SetToolbarVisibilityChangedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the number of toolbars has changed 
  // (i.e. a toolbar is added or removed).
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetNumberOfToolbarsChangedCommand(
    vtkObject *object, const char *method);

  // Description:
  // Set/Get the visibility of the separator at the bottom of the set
  virtual void SetBottomSeparatorVisibility(int);
  vtkBooleanMacro(BottomSeparatorVisibility, int); 
  vtkGetMacro(BottomSeparatorVisibility, int); 

  // Description:
  // Set/Get the visibility of the separator at the top of the set
  virtual void SetTopSeparatorVisibility(int);
  vtkBooleanMacro(TopSeparatorVisibility, int); 
  vtkGetMacro(TopSeparatorVisibility, int); 

  // Description:
  // Update the toolbar set 
  // (update the enabled state of all toolbars, call PackToolbars(), etc.).
  virtual void Update();

  // Description:
  // (Re)Pack the toolbars, if needed (if the widget is created, and the
  // toolbar is created, AddToolbar will pack the toolbar automatically).
  virtual void Pack();
  
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

  vtkKWSeparator *TopSeparator;
  vtkKWFrame     *ToolbarsFrame;
  vtkKWSeparator *BottomSeparator;

  int BottomSeparatorVisibility;
  int TopSeparatorVisibility;
  int SynchronizeToolbarsVisibilityWithRegistry;

  char *ToolbarVisibilityChangedCommand;
  char *NumberOfToolbarsChangedCommand;

  virtual void InvokeToolbarVisibilityChangedCommand(
    vtkKWToolbar *toolbar);
  virtual void InvokeNumberOfToolbarsChangedCommand();

  //BTX

  // A toolbar slot stores a toolbar + some infos
 
  class ToolbarSlot
  {
  public:
    int Visibility;
    int Anchor;
    vtkKWSeparator *Separator;
    vtkKWToolbar   *Toolbar;
  };

  // PIMPL Encapsulation for STL containers

  vtkKWToolbarSetInternals *Internals;
  friend class vtkKWToolbarSetInternals;

  // Helper methods

  ToolbarSlot* GetToolbarSlot(vtkKWToolbar *toolbar);

  //ETX

  virtual void PackToolbars();
  virtual void PackBottomSeparator();
  virtual void PackTopSeparator();

private:
  vtkKWToolbarSet(const vtkKWToolbarSet&); // Not implemented
  void operator=(const vtkKWToolbarSet&); // Not implemented
};

#endif

