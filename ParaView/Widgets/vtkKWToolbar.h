/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
  // Determines whether the toolbar is resizable. When you use Pack(),
  // do not provide options like -fill or -expand.
  virtual void SetResizable(int);
  vtkGetMacro(Resizable, int);
  vtkBooleanMacro(Resizable, int);

  // Description:
  // Callbacks to ensure all widgets are visible (only
  // if the were added with AddWidget)
  virtual void ScheduleResize();
  virtual void Resize();

  // Description:
  // Update/refresh the widgets layout/aspect
  virtual void UpdateWidgets();

  // Description:
  // Update/refresh the toolbar layout/aspect (does not include the widgets)
  virtual void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts subwidgets. This will, for example,
  // enable disable parts of the widget UI, enable disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Pack the toolbar. Do not use Tk 'pack' since the toolbar has more
  // fancy things to do after packing.
  virtual void Pack(const char*);
  virtual void Pack() { this->Pack(""); };

  // Description:
  // Add a widget to the toolbar, insert a widget before 'location' (or at
  // beginning of list if 'location' is not found), remove widget.
  virtual void AddWidget(vtkKWWidget* widget);
  virtual void InsertWidget(vtkKWWidget* location, vtkKWWidget* widget);
  virtual void RemoveWidget(vtkKWWidget* widget);
  virtual vtkKWWidget* GetWidget(const char *name);

  // Description:
  // Convenience method to create and add a specific type of widget 
  vtkKWWidget* AddRadioButtonImage(int value, 
                                   const char *image_name, 
                                   const char *select_image_name, 
                                   const char *variable_name,
                                   vtkKWObject *object, 
                                   const char *method, 
                                   const char *help = 0,
                                   const char *extra = 0);
  
  // Description:
  // Returns the main frame of the toolbar. Put all the widgets
  // in the main frame.
  vtkGetObjectMacro(Frame, vtkKWWidget);

  // Description:
  // Returns the separator of the toolbar. No real need to modify
  // it, but useful for testing.
  vtkGetObjectMacro(Separator, vtkKWWidget);

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

protected:
  vtkKWToolbar();
  ~vtkKWToolbar();

  // Height stuf is not working (ask ken)
  int Expanding;

  vtkKWWidget *Frame;
  vtkKWWidget *Separator;

  void ConstrainWidgetsLayout();
  void UpdateWidgetsLayout();
  void UpdateWidgetsAspect();
  
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



