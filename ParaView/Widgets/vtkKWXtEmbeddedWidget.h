/*=========================================================================

  Module:    vtkKWXtEmbeddedWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWXtEmbeddedWidget - dialog box superclass
// .SECTION Description
// A class that can be used to embed KW widgets in Motif or Xt windows.
// If the WindowId is not set, a new top level window is created.

#ifndef __vtkKWXtEmbeddedWidget_h
#define __vtkKWXtEmbeddedWidget_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWXtEmbeddedWidget : public vtkKWWidget
{
public:
  static vtkKWXtEmbeddedWidget* New();
  vtkTypeRevisionMacro(vtkKWXtEmbeddedWidget,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the window ID to embed the widget in.
  // If this is not set, a toplevel window is created
  void SetWindowId(void* w) { this->WindowId = w;}
  
  // Description:
  // Create create the widget.  If WindowId is set,
  // then the widget is placed in that window.  If it
  // is not set, then a toplevel window is created.
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // If the window id is null, this deiconifies the toplevel
  // window.
  virtual void Display();

protected:
  vtkKWXtEmbeddedWidget();
  ~vtkKWXtEmbeddedWidget();

  void* WindowId;
private:
  vtkKWXtEmbeddedWidget(const vtkKWXtEmbeddedWidget&); // Not implemented
  void operator=(const vtkKWXtEmbeddedWidget&); // Not implemented
};


#endif



