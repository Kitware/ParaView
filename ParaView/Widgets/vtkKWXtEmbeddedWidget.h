/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWXtEmbeddedWidget.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
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
  vtkTypeMacro(vtkKWXtEmbeddedWidget,vtkKWWidget);

  // Description:
  // Set the window ID to embed the widget in.
  // If this is not set, a toplevel window is created
  void SetWindowId(void* w) { this->WindowId = w;}
  
  // Description:
  // Create create the widget.  If WindowId is set,
  // then the widget is placed in that window.  If it
  // is not set, then a toplevel window is created.
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // If the window id is null, this deiconifies the toplevel
  // window.
  virtual void Display();

protected:
  vtkKWXtEmbeddedWidget();
  ~vtkKWXtEmbeddedWidget();
  vtkKWXtEmbeddedWidget(const vtkKWXtEmbeddedWidget&) {};
  void operator=(const vtkKWXtEmbeddedWidget&) {};

  void* WindowId;
};


#endif


