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

class VTK_EXPORT vtkKWToolbarSet : public vtkKWWidget
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
  // Return 1 on success, 0 otherwise.
  int AddToolbar(vtkKWToolbar *toolbar);
  int HasToolbar(vtkKWToolbar *toolbar);
  
  // Description:
  // Return a toolbar at a particular index.
  vtkKWToolbar* GetToolbar(int index);
  vtkIdType GetNumberOfToolbars();

  // Description:
  // Set/Get the flat aspect of the toolbars
  virtual void SetToolbarsFlatAspect(int);

  // Description:
  // Set/Get the flat aspect of the widgets (flat or 3D GUI style)
  virtual void SetToolbarsWidgetsFlatAspect(int);

  // Description:
  // Convenience method to hide/show a toolbar
  void HideToolbar(vtkKWToolbar *toolbar);
  void ShowToolbar(vtkKWToolbar *toolbar);
  void SetToolbarVisibility(vtkKWToolbar *toolbar, int flag);

  // Description:
  // Indicates if the given toolbar is visible in 
  // the toolset.
  int IsToolbarVisible(vtkKWToolbar *toolbar);
  
  vtkIdType GetNumberOfVisibleToolbars();

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

