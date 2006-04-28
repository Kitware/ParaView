/*=========================================================================

  Module:    vtkKWTopLevel.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTopLevel - toplevel superclass
// .SECTION Description
// A generic superclass for toplevel.

#ifndef __vtkKWTopLevel_h
#define __vtkKWTopLevel_h

#include "vtkKWCoreWidget.h"

class vtkKWMenu;

class KWWidgets_EXPORT vtkKWTopLevel : public vtkKWCoreWidget
{
public:
  static vtkKWTopLevel* New();
  vtkTypeRevisionMacro(vtkKWTopLevel,vtkKWCoreWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Class of the window. Used to group several windows under the same class
  // (they will, for example, de-iconify together).
  // Make sure you set it before a call to Create().
  vtkSetStringMacro(WindowClass);
  vtkGetStringMacro(WindowClass);

  // Description:
  // Set the widget/window to which this toplevel will be slave.
  // If set, this toplevel will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this).
  // For convenience purposes, the MasterWindow does not have to be a
  // toplevel, it can be a plain widget (its toplevel will be found
  // at runtime).
  // Has to be called before Create().
  virtual void SetMasterWindow(vtkKWWidget* win);
  vtkGetObjectMacro(MasterWindow, vtkKWWidget);

  // Description:
  // Get the application instance for this object.
  // Override the superclass to try to retrieve the masterwindow's application
  // if it was not set already.
  virtual vtkKWApplication* GetApplication();

  // Description:
  // Display the toplevel. Hide it with the Withdraw() method.
  // This also call DeIconify(), Focus() and Raise()
  virtual void Display();

  // Description:
  // Arranges for window to be withdrawn from the screen. This causes the
  // window to be unmapped and forgotten about by the window manager.
  virtual void Withdraw();

  // Description:
  // Inform the window manager that this toplevel should be modal.
  // If it is set, Display() will bring up the toplevel and grab it.
  // Withdraw() will bring down the toplevel, and release the grab.
  vtkSetClampMacro(Modal, int, 0, 1);
  vtkBooleanMacro(Modal, int);
  vtkGetMacro(Modal, int);

  // Description:
  // Set/Get the background color of the widget.
  virtual void GetBackgroundColor(double *r, double *g, double *b);
  virtual double* GetBackgroundColor();
  virtual void SetBackgroundColor(double r, double g, double b);
  virtual void SetBackgroundColor(double rgb[3])
    { this->SetBackgroundColor(rgb[0], rgb[1], rgb[2]); };
  
  // Description:
  // Set/Get the border width, a non-negative value indicating the width of
  // the 3-D border to draw around the outside of the widget (if such a border
  // is being drawn; the Relief option typically determines this).
  virtual void SetBorderWidth(int);
  virtual int GetBorderWidth();
  
  // Description:
  // Set/Get the highlight thickness, a non-negative value indicating the
  // width of the highlight rectangle to draw around the outside of the
  // widget when it has the input focus.
  virtual void SetHighlightThickness(int);
  virtual int GetHighlightThickness();
  
  // Description:
  // Set/Get the 3-D effect desired for the widget. 
  // The value indicates how the interior of the widget should appear
  // relative to its exterior. 
  // Valid constants can be found in vtkKWOptions::ReliefType.
  virtual void SetRelief(int);
  virtual int GetRelief();
  virtual void SetReliefToRaised();
  virtual void SetReliefToSunken();
  virtual void SetReliefToFlat();
  virtual void SetReliefToRidge();
  virtual void SetReliefToSolid();
  virtual void SetReliefToGroove();

  // Description:
  // Set/Get the padding that will be applied around each widget (in pixels).
  // Specifies a non-negative value indicating how much extra space to request
  // for the widget in the X and Y-direction. When computing how large a
  // window it needs, the widget will add this amount to the width it would
  // normally need (as determined by the width of the things displayed
  // in the widget); if the geometry manager can satisfy this request, the 
  // widget will end up with extra internal space around what it displays 
  // inside. 
  virtual void SetPadX(int);
  virtual int GetPadX();
  virtual void SetPadY(int);
  virtual int GetPadY();

  // Description:
  // Set/Get the position this toplevel should be centered at when Display()
  // is called. The default setting, Default, is to not set/change the
  // position at all and let the user or the window manager place the toplevel.
  // If set to MasterWindowCenter, the toplevel is centered inside its master 
  // window ; if  the MasterWindow ivar is not set, it is centered on the
  // screen, which is similar to the ScreenCenter setting. If set to 
  // Pointer, the toplevel is centered at the current mouse position.
  // On some sytem, the default setting can lead the window manager to
  // place the window at the upper left corner (0, 0) the first time it
  // is displayed. Since this can be fairly annoying, the 
  // MasterWindowCenterFirst and ScreenCenterFirst can be used to center
  // the toplevel relative to the master window or the screen only the
  // first time it is displayed (after that, the toplevel will be displayed
  // wherever it was left).
  //BTX
  enum
  {
    DisplayPositionDefault                 = 0,
    DisplayPositionMasterWindowCenter      = 1,
    DisplayPositionMasterWindowCenterFirst = 2,
    DisplayPositionScreenCenter            = 3,
    DisplayPositionScreenCenterFirst       = 4,
    DisplayPositionPointer                 = 5
  };
  //ETX
  vtkSetClampMacro(DisplayPosition, int, 
                   vtkKWTopLevel::DisplayPositionDefault, 
                   vtkKWTopLevel::DisplayPositionPointer);
  vtkGetMacro(DisplayPosition, int);
  virtual void SetDisplayPositionToDefault() 
    { this->SetDisplayPosition(
      vtkKWTopLevel::DisplayPositionDefault); };
  virtual void SetDisplayPositionToMasterWindowCenter() 
    { this->SetDisplayPosition(
      vtkKWTopLevel::DisplayPositionMasterWindowCenter); };
  virtual void SetDisplayPositionToMasterWindowCenterFirst() 
    { this->SetDisplayPosition(
      vtkKWTopLevel::DisplayPositionMasterWindowCenterFirst); };
  virtual void SetDisplayPositionToScreenCenter() 
    { this->SetDisplayPosition(
      vtkKWTopLevel::DisplayPositionScreenCenter); };
  virtual void SetDisplayPositionToScreenCenterFirst() 
    { this->SetDisplayPosition(
      vtkKWTopLevel::DisplayPositionScreenCenterFirst); };
  virtual void SetDisplayPositionToPointer() 
    { this->SetDisplayPosition(
      vtkKWTopLevel::DisplayPositionPointer); };

  // Description:
  // Arrange for the toplevel to be displayed in normal (non-iconified) form.
  // This is done by mapping the window.
  virtual void DeIconify();

  // Description:
  // Set the title of the toplevel.
  virtual void SetTitle(const char *);
  vtkGetStringMacro(Title);

  // Description:
  // Set the title to the same title as another widget's toplevel.
  virtual void SetTitleToTopLevelTitle(vtkKWWidget*);

  // Description:
  // Set/Get the window position in screen pixel coordinates. No effect if
  // called before Create()
  // Return 1 on success, 0 otherwise.
  virtual int SetPosition(int x, int y);
  virtual int GetPosition(int *x, int *y);

  // Description:
  // Set/Get the window size in pixels.  No effect if called before Create()
  // This will in turn call GetWidget() and GetHeight()
  // Return 1 on success, 0 otherwise.
  virtual int SetSize(int w, int h);
  virtual int GetSize(int *w, int *h);

  // Description:
  // Get the width/height of the toplevel.
  virtual int GetWidth();
  virtual int GetHeight();

  // Description:
  // Set/Get the minimum window size. 
  // For gridded windows the dimensions are specified in grid units; 
  // otherwise they are specified in pixel units. The window manager will
  // restrict the window's dimensions to be greater than or equal to width
  // and height.
  // No effect if called before Create()
  // Return 1 on success, 0 otherwise.
  virtual int SetMinimumSize(int w, int h);
  virtual int GetMinimumSize(int *w, int *h);

  // Description:
  // Set/Get the window size and position in screen pixel
  // coordinates as a geometry format wxh+x+y (ex: 800x700+20+50). 
  // No effect if called before Create()
  // SetGeometry will return 1 on success, 0 otherwise.
  // GetGeometry will return the geometry in a temporary buffer on success
  // (copy the value to another string buffer as soon as possible), or NULL
  // otherwise
  virtual int SetGeometry(const char *);
  virtual const char* GetGeometry();

  // Description:
  // Set/Get if the toplevel should be displayed without decorations (i.e.
  // ignored by the window manager). Default to 0. If not decorated, the
  // toplevel will usually be displayed without a title bar, resizing handles,
  // etc.
  virtual void SetHideDecoration(int);
  vtkGetMacro(HideDecoration, int);
  vtkBooleanMacro(HideDecoration, int);

  // Description:
  // Get the menu associated to this toplevel.
  // Note that this menu is created on the fly to lower the footprint
  // of this object. 
  vtkKWMenu *GetMenu();

  // Description:
  // Specifies a command to associate with the widget. This command is 
  // typically invoked when the user closes the window using the
  // window manager.
  // The 'object' argument is the object that will have the method called on
  // it. The 'method' argument is the name of the method to be called and any
  // arguments in string form. If the object is NULL, the method is still
  // evaluated as a simple command. 
  virtual void SetDeleteWindowProtocolCommand(
    vtkObject *object, const char *method);

  // Description:
  // Set the name inside the icon associated to this window/toplevel.
  virtual void SetIconName(const char *name);

  // Description:
  // Set whether or not the user may interactively resize the toplevel window.
  // The parameters are boolean values that determine whether the width and
  // height of the window may be modified by the user.
  // No effect if called before Create()
  virtual void SetResizable(int w, int h);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWTopLevel();
  ~vtkKWTopLevel();

  // Description:
  // Create the widget.
  // Make sure WindowClass is set before calling this method (if needed).
  // If MasterWindow is set and is a vtkKWTopLevel, its class will be used
  // to set our own WindowClass.
  // Withdraw() is called at the end of the creation.
  virtual void CreateWidget();

  vtkKWWidget *MasterWindow;
  vtkKWMenu   *Menu;

  char *Title;
  char *WindowClass;

  int HideDecoration;
  int Modal;
  int DisplayPosition;

  // Description:
  // Get the width/height of the toplevel as requested
  // by the window manager. Not exposed in public since it is so Tk
  // related. Is is usually used to get the geometry of a window before
  // it is mapped to screen, as requested by the geometry manager.
  virtual int GetRequestedWidth();
  virtual int GetRequestedHeight();

  // Description:
  // Compute the display position (centered or at pointer)
  // Return 1 on success, 0 otherwise
  virtual int ComputeDisplayPosition(int *x, int *y);

  // Description:
  // Setup transient, protocol, title and other settings right after
  // the widget has been created. This can be used by subclass that
  // really need to create the toplevel manually, but want to have
  // the ivar setup properly
  virtual void PostCreate();

private:
  vtkKWTopLevel(const vtkKWTopLevel&); // Not implemented
  void operator=(const vtkKWTopLevel&); // Not Implemented
};

#endif
