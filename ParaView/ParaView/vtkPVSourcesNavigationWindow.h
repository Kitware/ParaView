/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVSourcesNavigationWindow.h
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
// .NAME vtkPVSourcesNavigationWindow - Widget for PV sources and their inputs and outputs
// .SECTION Description
// vtkPVSourcesNavigationWindow is a specialized ParaView widget used for
// displaying a local presentation of the underlying pipeline. It
// allows the user to navigate by clicking on the appropriate tags.

#ifndef __vtkPVSourcesNavigationWindow_h
#define __vtkPVSourcesNavigationWindow_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWWidget;
class vtkPVSource;
class vtkKWMenu;

class VTK_EXPORT vtkPVSourcesNavigationWindow : public vtkKWWidget
{
public:
  static vtkPVSourcesNavigationWindow* New();
  vtkTypeMacro(vtkPVSourcesNavigationWindow,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the width and the height of the underlying canvas
  void SetWidth(int width);
  void SetHeight(int height);

  // Description:
  // Return the underlying canvas
  vtkGetObjectMacro(Canvas, vtkKWWidget);

  // Description:
  // Regenerate the display and re-assign bindings.
  void Update(vtkPVSource *currentSource);

  // Description:
  // Highlight the object.
  void HighlightObject(const char* widget, int onoff);

  // Description:
  // Display the module popup menu
  void DisplayModulePopupMenu(const char*, int x, int y);

  // Description:
  // Execute a command on module.
  void ExecuteCommandOnModule(const char* module, const char* command);
  
  // Description:
  // This method is called before the object is deleted.
  virtual void PrepareForDelete() {}

  // Description:
  // This method is called when canvas size changes.
  virtual void Reconfigure();
 
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

//BTX
  const char* CreateCanvasItem(const char *format, ...);
//ETX
  
  int Width;
  int Height;
  vtkKWWidget* Canvas;
  vtkKWWidget* ScrollBar;
  vtkKWMenu* PopupMenu;

private:
  vtkPVSourcesNavigationWindow(const vtkPVSourcesNavigationWindow&); // Not implemented
  void operator=(const vtkPVSourcesNavigationWindow&); // Not Implemented
};


#endif


