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

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWRadioButton;

//BTX
template <class value>
class vtkVector;
//ETX

class VTK_EXPORT vtkKWToolbar : public vtkKWWidget
{
public:
  static vtkKWToolbar* New();
  vtkTypeRevisionMacro(vtkKWToolbar, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Returns the main frame of the toolbar. Put all the widgets
  // in the main frame.
  vtkGetObjectMacro(Frame, vtkKWFrame);

  // Description:
  // Determines whether the toolbar is resizable. 
  virtual void SetResizable(int);
  vtkGetMacro(Resizable, int);
  vtkBooleanMacro(Resizable, int);

  // Description:
  // Add a widget to the toolbar, insert a widget before 'location' (or at
  // beginning of list if 'location' is not found), remove widget.
  virtual void AddWidget(vtkKWWidget* widget);
  virtual void InsertWidget(vtkKWWidget* location, vtkKWWidget* widget);
  virtual void RemoveWidget(vtkKWWidget* widget);
  virtual vtkKWWidget* GetWidget(const char *name);

  // Description:
  // Convenience method to create and add a specific type of widget 
  // Note: for radiobutton, the variable_name should be the same for
  //       each radiobutton in the set of radiobuttons.
  //       for checkbutton, this is only optional (can be NULL)
  vtkKWWidget* AddRadioButtonImage(int value, 
                                   const char *image_name, 
                                   const char *select_image_name, 
                                   const char *variable_name,
                                   vtkKWObject *object, 
                                   const char *method, 
                                   const char *help = 0,
                                   const char *extra = 0);
  vtkKWWidget* AddCheckButtonImage(const char *image_name, 
                                   const char *select_image_name, 
                                   const char *variable_name,
                                   vtkKWObject *object, 
                                   const char *method, 
                                   const char *help = 0,
                                   const char *extra = 0);
  
  // Description:
  // Update/refresh the widgets layout/aspect
  virtual void UpdateWidgets();

  // Description:
  // Update/refresh the toolbar layout/aspect (does not include the widgets)
  virtual void Update();

  // Description:
  // Set/Get the flat aspect of the toolbar (flat or 3D GUI style)
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
  // Set/Get the internal padding that will be applied around each widget.
  // (default to 0 on Windows, 1 otherwise).
  virtual void SetPadX(int);
  vtkGetMacro(PadX, int);
  virtual void SetPadY(int);
  vtkGetMacro(PadY, int);

  // Description:
  // Set/Get the additional internal padding that will be applied around 
  // each widget when WidgetsFlatAspect is On (default to 1).
  virtual void SetWidgetsFlatAdditionalPadX(int);
  vtkGetMacro(WidgetsFlatAdditionalPadX, int);
  virtual void SetWidgetsFlatAdditionalPadY(int);
  vtkGetMacro(WidgetsFlatAdditionalPadY, int);

  // Description:
  // Callbacks to ensure all widgets are visible (only
  // if the were added with AddWidget)
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

protected:
  vtkKWToolbar();
  ~vtkKWToolbar();

  // Height stuf is not working (ask ken)
  int Expanding;

  vtkKWFrame *Frame;
  vtkKWFrame *Handle;

  void ConstrainWidgetsLayout();
  void UpdateWidgetsLayout();
  void UpdateWidgetsAspect();
  void UpdateToolbarFrameAspect();
  
//BTX
  vtkVector<vtkKWWidget*>* Widgets;
//ETX

  int PadX;
  int PadY;
  int WidgetsFlatAdditionalPadX;
  int WidgetsFlatAdditionalPadY;

  int FlatAspect;
  int WidgetsFlatAspect;
  int Resizable;

  vtkKWRadioButton *DefaultOptionsWidget;

private:
  vtkKWToolbar(const vtkKWToolbar&); // Not implemented
  void operator=(const vtkKWToolbar&); // Not implemented
};


#endif



