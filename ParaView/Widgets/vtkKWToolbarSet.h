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
// .NAME vtkKWToolbarSet - a "set of toolbars" widget
// .SECTION Description
// A simple widget representing a set of toolbars..

#ifndef __vtkKWToolbarSet_h
#define __vtkKWToolbarSet_h

#include "vtkKWWidget.h"

class vtkKWApplication;
class vtkKWFrame;
class vtkKWToolbar;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWToolbarSet : public vtkKWWidget
{
public:
  static vtkKWToolbarSet* New();
  vtkTypeRevisionMacro(vtkKWToolbarSet,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget (a frame holding all the toolbars).
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Get the frame that can be used as a parent to a toolbar
  vtkGetObjectMacro(ToolbarsFrame, vtkKWFrame);

  // Description:
  // Add a toolbar to the set.
  // Return 1 on success, 0 otherwise.
  int AddToolbar(vtkKWToolbar *toolbar);
  int HasToolbar(vtkKWToolbar *toolbar);
  vtkIdType GetNumberOfToolbars();

  // Description:
  // Set/Get the flat aspect of the toolbars
  virtual void SetToolbarsFlatAspect(int);

  // Description:
  // Set/Get the flat aspect of the widgets (flat or 3D GUI style)
  virtual void SetToolbarsWidgetsFlatAspect(int);

  // Description:
  // Convenience method to hide/show a toolbar
  void HideToolbar(vtkKWToolbar *toolbar);
  void ShowToolbar(vtkKWToolbar *toolbar);
  void SetToolbarVisibility(vtkKWToolbar *toolbar, int flag);
  vtkIdType GetNumberOfVisibleToolbars();

  // Description:
  // Show or hide a separator at the bottom of the set
  virtual void SetShowBottomSeparator(int);
  vtkBooleanMacro(ShowBottomSeparator, int); 
  vtkGetMacro(ShowBottomSeparator, int); 

  // Description:
  // Update the toolbar set 
  // (update the enabled state of all toolbars, call PackToolbars(), etc.).
  virtual void Update();

  // Description:
  // (Re)Pack the toolbars, if needed (if the widget is created, and the
  // toolbar is created, AddToolbar will pack the toolbar automatically).
  virtual void PackToolbars();
  
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWToolbarSet();
  ~vtkKWToolbarSet();

  vtkKWFrame *ToolbarsFrame;
  vtkKWFrame *BottomSeparatorFrame;

  int ShowBottomSeparator;

  //BTX

  // A toolbar slot stores a toolbar + some infos
 
  class ToolbarSlot
  {
  public:
    int Visibility;
    vtkKWFrame   *SeparatorFrame;
    vtkKWToolbar *Toolbar;
  };

  typedef vtkLinkedList<ToolbarSlot*> ToolbarsContainer;
  typedef vtkLinkedListIterator<ToolbarSlot*> ToolbarsContainerIterator;
  ToolbarsContainer *Toolbars;

  // Helper methods

  ToolbarSlot* GetToolbarSlot(vtkKWToolbar *toolbar);

  //ETX

  virtual void Pack();
  virtual void PackBottomSeparator();

private:
  vtkKWToolbarSet(const vtkKWToolbarSet&); // Not implemented
  void operator=(const vtkKWToolbarSet&); // Not implemented
};

#endif

