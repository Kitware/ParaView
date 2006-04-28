/*=========================================================================

  Module:    vtkKWToolbar.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWToolbar - a frame that holds tool buttons
// .SECTION Description
// Simply a frame to hold a bunch of tools.  It uses bindings to control
// the height of the frame.
// In the future we could use the object to move toolbars groups around.

#ifndef __vtkKWToolbar_h
#define __vtkKWToolbar_h

#include "vtkKWCompositeWidget.h"

class vtkKWFrame;
class vtkKWRadioButton;
class vtkKWToolbarInternals;

class KWWidgets_EXPORT vtkKWToolbar : public vtkKWCompositeWidget
{
public:
  static vtkKWToolbar* New();
  vtkTypeRevisionMacro(vtkKWToolbar, vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Returns the main frame of the toolbar. 
  // This should be used as the parent of all the widgets in the toolbar.
  vtkGetObjectMacro(Frame, vtkKWFrame);

  // Description:
  // Determines whether the toolbar is resizable. 
  virtual void SetResizable(int);
  vtkGetMacro(Resizable, int);
  vtkBooleanMacro(Resizable, int);

  // Description:
  // Set/Get the name of the toolbar. This is optional but certainly
  // useful if this toolbar is meant to be added to a vtkKWToolbarSet
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // Add a widget to the toolbar, insert a widget before 'location' (or at
  // beginning of list if 'location' is not found)
  virtual void AddWidget(vtkKWWidget* widget);
  virtual void InsertWidget(vtkKWWidget* location, vtkKWWidget* widget);

  // Description:
  // Query widgets
  virtual int HasWidget(vtkKWWidget* widget);
  virtual int GetNumberOfWidgets();

  // Description:
  // Remove a widget (or all) from the toolbar
  virtual void RemoveWidget(vtkKWWidget* widget);
  virtual void RemoveAllWidgets();

  // Description:
  // Retrieve a widget given its name. The name is looked up in common Tk
  // options like -label, -text, -image, -selectimage
  virtual vtkKWWidget* GetWidget(const char *name);

  // Description:
  // Retrieve the nth- widget
  virtual vtkKWWidget* GetNthWidget(int rank);

  // Description:
  // Create and add a specific type of widget.
  // Note: for radiobutton, the variable_name should be the same for
  //       each radiobutton in the set of radiobuttons.
  //       for checkbutton, this is only optional (can be NULL)
  vtkKWWidget* AddRadioButtonImage(int value, 
                                   const char *image_name, 
                                   const char *select_image_name, 
                                   const char *variable_name,
                                   vtkObject *object, 
                                   const char *method, 
                                   const char *help = 0);
  vtkKWWidget* AddCheckButtonImage(const char *image_name, 
                                   const char *select_image_name, 
                                   const char *variable_name,
                                   vtkObject *object, 
                                   const char *method, 
                                   const char *help = 0);
  
  // Description:
  // Update/refresh the widgets layout/aspect
  virtual void UpdateWidgets();

  // Description:
  // Update/refresh the toolbar layout/aspect (does not include the widgets)
  virtual void Update();

  // Description:
  // Set/Get the flat aspect of the toolbar (flat or 3D GUI style)
  // The static GlobalFlatAspect member can be set so that all toolbars
  // are rendered using the same aspect.
  virtual void SetFlatAspect(int);
  vtkBooleanMacro(FlatAspect, int);
  vtkGetMacro(FlatAspect, int);
  static int GetGlobalFlatAspect();
  static void SetGlobalFlatAspect(int val);
  static void GlobalFlatAspectOn() 
    { vtkKWToolbar::SetGlobalFlatAspect(1); };
  static void GlobalFlatAspectOff() 
    { vtkKWToolbar::SetGlobalFlatAspect(0); };

  // Description:
  // Set/Get the flat aspect of the widgets (flat or 3D GUI style)
  // The static GlobalWidgetsFlatAspect member can be set so that all widgets
  // are rendered using the same aspect.
  virtual void SetWidgetsFlatAspect(int);
  vtkBooleanMacro(WidgetsFlatAspect, int);
  vtkGetMacro(WidgetsFlatAspect, int);
  static int GetGlobalWidgetsFlatAspect();
  static void SetGlobalWidgetsFlatAspect(int val);
  static void GlobalWidgetsFlatAspectOn() 
    { vtkKWToolbar::SetGlobalWidgetsFlatAspect(1); };
  static void GlobalWidgetsFlatAspectOff() 
    { vtkKWToolbar::SetGlobalWidgetsFlatAspect(0); };

  // Description:
  // Set/Get the padding that will be applied around each widget.
  // (default to 0 on Windows, 1 otherwise).
  virtual void SetWidgetsPadX(int);
  vtkGetMacro(WidgetsPadX, int);
  virtual void SetWidgetsPadY(int);
  vtkGetMacro(WidgetsPadY, int);

  // Description:
  // Set/Get the additional internal padding that will be applied around 
  // each widget when WidgetsFlatAspect is On (default to 1).
  virtual void SetWidgetsFlatAdditionalPadX(int);
  vtkGetMacro(WidgetsFlatAdditionalPadX, int);
  virtual void SetWidgetsFlatAdditionalPadY(int);
  vtkGetMacro(WidgetsFlatAdditionalPadY, int);

  // Description:
  // Schedule the widget to resize itself, or resize it right away
  virtual void ScheduleResize();
  virtual void Resize();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts subwidgets. This will, for example,
  // enable disable parts of the widget UI, enable disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Some constants
  //BTX
  static const char *FlatAspectRegKey;
  static const char *WidgetsFlatAspectRegKey;
  //ETX

protected:
  vtkKWToolbar();
  ~vtkKWToolbar();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  int Expanding;

  vtkKWFrame *Frame;
  vtkKWFrame *Handle;

  void ConstrainWidgetsLayout();
  void UpdateWidgetsLayout();
  void UpdateWidgetsAspect();
  void UpdateToolbarFrameAspect();
  
  //BTX

  // PIMPL Encapsulation for STL containers

  vtkKWToolbarInternals *Internals;

  //ETX

  int WidgetsPadX;
  int WidgetsPadY;
  int WidgetsFlatAdditionalPadX;
  int WidgetsFlatAdditionalPadY;

  int FlatAspect;
  int WidgetsFlatAspect;
  int Resizable;

  vtkKWRadioButton *DefaultOptionsWidget;

  char *Name;

  // Description:
  // Bind/Unbind events.
  virtual void Bind();
  virtual void UnBind();

private:
  vtkKWToolbar(const vtkKWToolbar&); // Not implemented
  void operator=(const vtkKWToolbar&); // Not implemented
};


#endif



