/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourcesNavigationWindow.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVSourcesNavigationWindow - Widget for PV sources and their inputs and outputs
// .SECTION Description
// vtkPVSourcesNavigationWindow is a specialized ParaView widget used for
// displaying a local presentation of the underlying pipeline. It
// allows the user to navigate by clicking on the appropriate tags.

#ifndef __vtkPVSourcesNavigationWindow_h
#define __vtkPVSourcesNavigationWindow_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWCanvas;
class vtkKWMenu;
class vtkKWWidget;
class vtkPVSource;

class VTK_EXPORT vtkPVSourcesNavigationWindow : public vtkKWWidget
{
public:
  static vtkPVSourcesNavigationWindow* New();
  vtkTypeRevisionMacro(vtkPVSourcesNavigationWindow,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the width and the height of the underlying canvas
  void SetWidth(int width);
  void SetHeight(int height);

  // Description:
  // Return the underlying canvas
  vtkGetObjectMacro(Canvas, vtkKWCanvas);

  // Description:
  // Regenerate the display and re-assign bindings.
  void Update(vtkPVSource *currentSource);

  // Description:
  // Highlight the object.
  void HighlightObject(const char* widget, int onoff);

  // Description:
  // Display the module popup menu
  void DisplayModulePopupMenu(vtkPVSource* module, int x, int y);

  // Description:
  // Callbacks for the popup menu.
  void PopupDeleteCallback();
  void PopupVisibilityCallback();
  void PopupFlatInterpolationCallback();
  void PopupGouraudInterpolationCallback();
  void PopupOutlineRepresentationCallback();
  void PopupSurfaceRepresentationCallback();
  void PopupWireframeRepresentationCallback();
  void PopupPointsRepresentationCallback();
  
  // Description:
  // Execute a command on module.
  void ExecuteCommandOnModule(const char* module, const char* command);
  
  // Description:
  // This method is called before the object is deleted.
  virtual void PrepareForDelete() {}

  // Description:
  // This method is called when canvas size changes.
  virtual void Reconfigure();
 
  // Description:
  // Enable/disable the fact that the source name will always be displayed
  // even if the description is not empty.
  virtual void SetAlwaysShowName(int);
  vtkGetMacro(AlwaysShowName, int);
  vtkBooleanMacro(AlwaysShowName, int);
 
  // Description:
  // Enable/disable the fact that selection bindings are set for each entry
  // in the canvas. It does *not* create these bindings, they will be 
  // created or removed by the next call to Update() or ChildUpdate().
  vtkSetMacro(CreateSelectionBindings, int);
  vtkGetMacro(CreateSelectionBindings, int);
  vtkBooleanMacro(CreateSelectionBindings, int);
 
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkPVSourcesNavigationWindow();
  ~vtkPVSourcesNavigationWindow();

  // Description:
  // This method calculates the bounding box of object "name". 
  void CalculateBBox(vtkKWWidget* canvas, const char* name, int bbox[4]);

  // Description:
  // This method is called at beginning of the Update method. The
  // subclass is supposed to overwrite it.
  virtual void ChildUpdate(vtkPVSource* currentSource);

  // Description:
  // This method is called at the end of Update method. If the
  // subclass needs any special setup after update, it should
  // overwrite this method.
  virtual void PostChildUpdate() {}

  // Description:
  // This method is called at the end of the Create method. If the
  // subclass needs any special setup, it should overwrite this
  // method.
  virtual void ChildCreate() {}

  // Description:
  // Return the textual representation of the composite (i.e. its name and/or
  // its description. Memory is allocated, a pointer is return, it's up to
  // the caller to delete it.
  char* GetTextRepresentation(vtkPVSource* comp);

//BTX
  const char* CreateCanvasItem(const char *format, ...);
//ETX

  int Width;
  int Height;
  vtkKWCanvas* Canvas;
  vtkKWWidget* ScrollBar;
  vtkKWMenu* PopupMenu;
  // Set while the popup is active so we know which module to modify.
  vtkPVSource* PopupModule;

  int AlwaysShowName;
  int CreateSelectionBindings;

private:
  vtkPVSourcesNavigationWindow(const vtkPVSourcesNavigationWindow&); // Not implemented
  void operator=(const vtkPVSourcesNavigationWindow&); // Not Implemented
};


#endif
