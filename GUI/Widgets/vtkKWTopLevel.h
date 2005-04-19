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

#include "vtkKWWidget.h"

class vtkKWMenu;

class VTK_EXPORT vtkKWTopLevel : public vtkKWWidget
{
public:
  static vtkKWTopLevel* New();
  vtkTypeRevisionMacro(vtkKWTopLevel,vtkKWWidget);
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
  virtual void SetMasterWindow(vtkKWWidget* win);
  vtkGetObjectMacro(MasterWindow, vtkKWWidget);

  // Description:
  // Create the widget.
  // Make sure WindowClass is set before calling this method (if needed).
  // If MasterWindow is set and is a vtkKWTopLevel, its class will be used
  // to set our own WindowClass.
  // Withdraw() is called at the end of the creation.
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Display the toplevel.
  // This also call DeIconify(), Focus() and Raise()
  virtual void Display();

  // Description:
  // Arrange for the toplevel to be displayed in normal (non-iconified) form.
  // This is done by mapping the window.
  virtual void DeIconify();

  // Description:
  // Arranges for window to be withdrawn from the screen. This causes the
  // window to be unmapped and forgotten about by the window manager.
  virtual void Withdraw();

  // Description:
  // Arranges for window to be displayed above all of its siblings in the
  // stacking order.
  virtual void Raise();

  // Description:
  // Set the title of the toplevel.
  virtual void SetTitle(const char *);
  vtkGetStringMacro(Title);

  // Description:
  // Convenience method to set/get the window position in screen pixel
  // coordinates. No effect if called before Create()
  // Return 1 on success, 0 otherwise.
  virtual int SetPosition(int x, int y);
  virtual int GetPosition(int *x, int *y);

  // Description:
  // Convenience method to set/get the window size in pixels 
  // No effect if called before Create()
  // Return 1 on success, 0 otherwise.
  virtual int SetSize(int w, int h);
  virtual int GetSize(int *w, int *h);

  // Description:
  // Convenience method to guess the width/height of the toplevel.
  virtual int GetWidth();
  virtual int GetHeight();

  // Description:
  // Return if the toplevel has ever been mapped (deiconified)
  vtkGetMacro(HasBeenMapped, int);

  // Description:
  // Set/Get if the toplevel should be displayed without decorations (i.e.
  // ignored by the window manager). Default to 0. If not decorated, the
  // toplevel will usually be displayed without a title bar, resizing handles,
  // etc.
  virtual void SetHideDecoration(int);
  vtkGetMacro(HideDecoration, int);
  vtkBooleanMacro(HideDecoration, int);

  // Description:
  // Install a menu bar into this toplevel.
  // Note that it has no effect before a call to Create().
  // The menu has to be Create()'ed too.
  virtual void InstallMenu(vtkKWMenu *menu);

protected:

  vtkKWTopLevel();
  ~vtkKWTopLevel();

  vtkKWWidget* MasterWindow;

  char *Title;
  char *WindowClass;

  int HasBeenMapped;
  int HideDecoration;

private:
  vtkKWTopLevel(const vtkKWTopLevel&); // Not implemented
  void operator=(const vtkKWTopLevel&); // Not Implemented
};

#endif
