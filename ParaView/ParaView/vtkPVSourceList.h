/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSourceList.h
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
// .NAME vtkPVSourceList
// .SECTION Description
// This is the UI for the Assembly browser / editor.

#ifndef __vtkPVSourceList_h
#define __vtkPVSourceList_h

#include "vtkKWWidget.h"

class vtkKWEntry;
class vtkPVSource;
class vtkPVSourceCollection;
class vtkKWMenu;

class VTK_EXPORT vtkPVSourceList : public vtkKWWidget
{
public:
  static vtkPVSourceList* New();
  vtkTypeMacro(vtkPVSourceList,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // Redraws the canvas (assembly or list changed).
  void Update(vtkPVSource*, vtkPVSourceCollection*);

  // Description:
  // Callback from the canvas buttons.
  void Pick(int assyIdx);
  void ToggleVisibility(int assyIdx, int button);
  void EditColor(int assyIdx);

  // Description:
  // Callbacks from menu items.
  void DeletePicked();
  void DeletePickedVerify();

  // Description:
  // Set the height of the widget.
  void SetHeight(int height);

  // Description:
  // Display the module popup menu
  void DisplayModulePopupMenu(const char*, int x, int y); 
 
  // Description:
  // Execute a command on module.
  void ExecuteCommandOnModule(const char* module, const char* command);

  // Description:
  // This method is called before the object is deleted.
  void PrepareForDelete();
 
protected:
  vtkPVSourceList();
  ~vtkPVSourceList();

  int Update(vtkPVSource *comp, int y, int in, int current);

  vtkKWWidget *ScrollFrame;
  vtkKWWidget *Canvas;
  vtkKWWidget *ScrollBar;
  vtkKWMenu* PopupMenu;

  int Height;
  
  // Description:
  // The assembly that is displayed in the editor.
  vtkGetObjectMacro(Sources, vtkPVSourceCollection);
  virtual void SetSources(vtkPVSourceCollection*);
  vtkPVSourceCollection *Sources;

  vtkPVSource* CurrentSource;

private:
  vtkPVSourceList(const vtkPVSourceList&); // Not implemented
  void operator=(const vtkPVSourceList&); // Not implemented
};

#endif


