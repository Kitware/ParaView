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
// .NAME vtkKWUserInterfacePanel - a user interface panel.
// .SECTION Description
// This class is used to abstract the way an interface "panel" can be 
// subdivided into "pages" (i.e. "sections"). It allows specific GUI parts of
// an application to be encapsulated inside independent panels. Panels are 
// then associated to a user interface manager (see vtkKWUserInterfaceManager)
// which is responsible for grouping them inside a widget and handling user 
// interaction so that panels and their pages can be selected in more or less
// fancy ways. If the user interface manager uses a notebook under the hood, 
// then this class is likely to receive a notebook's page when it will request
// for a page from the manager. If the manager chooses for a flat GUI, then 
// this class is likely to receive a simple frame that will be stacked by the
// manager on top of other pages.
// This class is not a widget, it can not be mapped, the manager is the
// place where a concrete widget is set and used as the root of all panels (see
// vtkKWUserInterfaceNotebookManager for example). What you need to do
// is to set the UserInterfaceManager's Ivar to a manager, and the rest should
// be taken care of (i.e. the panel is automatically added to the manager, 
// and if the panel is not created the first time one if its pages is shown or 
// raised, the panel's Create() method is automatically called by the manager, 
// allowing the creation of the panel to be delayed until it is really needed).
// You should not use the manager's API to add the panel, add or raise pages,
// etc, just use this panel's API and calls will be propagated to the 
// right manager with the proper arguments).
// .SECTION See Also
// vtkKWUserInterfaceManager vtkKWUserInterfaceNotebookManager

#ifndef __vtkKWUserInterfacePanel_h
#define __vtkKWUserInterfacePanel_h

#include "vtkKWObject.h"

class vtkKWApplication;
class vtkKWIcon;
class vtkKWUserInterfaceManager;
class vtkKWWidget;

class VTK_EXPORT vtkKWUserInterfacePanel : public vtkKWObject
{
public:
  static vtkKWUserInterfacePanel* New();
  vtkTypeRevisionMacro(vtkKWUserInterfacePanel,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the user interface manager. This automatically adds/registers the 
  // panel to the manager. If you want to remove this panel from the manager,
  // set the manager to NULL (it is done automatically by the destructor).
  virtual void SetUserInterfaceManager(vtkKWUserInterfaceManager*);
  vtkGetObjectMacro(UserInterfaceManager, vtkKWUserInterfaceManager);

  // Description:
  // Set the panel name. Can be used to add the panel to a menu, etc.
  vtkGetStringMacro(Name);
  vtkSetStringMacro(Name);

  // Description:
  // Create the interface objects. Note that if the panel is not created 
  // the first time one if its pages is shown or raised, this method is 
  // automatically called by the manager, allowing the creation of the 
  // panel to be delayed until it is really needed. In the same way, if
  // the user interface manager has not been created at this point, it
  // is automatically created now (see vtkKWUserInterfaceManager::Create()).
  virtual void Create(vtkKWApplication *app);
  virtual int IsCreated() { return (this->Application != 0); }

  // Description:
  // Enable/Disable this panel. This should propagate SetEnabled() calls to the
  // internal widgets.
  virtual void SetEnabled(int);
  vtkBooleanMacro(Enabled, int);
  vtkGetMacro(Enabled, int);

  // Description:
  // Add a page to the panel (this will, in turn, instructs the manager to 
  // reserve a page for this given panel).
  // Return a unique positive ID, or < 0 on error.
  virtual int AddPage(const char *title, 
                      const char *balloon = 0, 
                      vtkKWIcon *icon = 0);

  // Description:
  // Retrieve the widget corresponding to a given page added to the panel.
  // This can be done through the unique page ID, or using the page title. 
  // The user UI components should be inserted into this widget.
  // Return NULL on error.
  virtual vtkKWWidget *GetPageWidget(int id);
  virtual vtkKWWidget *GetPageWidget(const char *title);

  // Description:
  // Retrieve the parent widget of the pages associated to the panel. It is
  // the unique widget that is common to all pages in the chain of parents.
  virtual vtkKWWidget *GetPagesParentWidget();

  // Description:
  // Raise a page added to the panel. This can be done through the unique 
  // page ID, or using the page title. Note that if the panel has not been
  // created at this point, the manager will call the panel's Create() 
  // method automatically, allowing the creation of the panel to be delayed
  // until it is really needed.
  virtual void RaisePage(int id);
  virtual void RaisePage(const char *title);

  // Description:
  // Show a panel. It will make sure the pages added to this panel are shown.
  // Note that if the panel has not been created at this point, the manager 
  // will call the panel's Create() method automatically, allowing the 
  // creation of the panel to be delayed until it is really needed.
  // Raise() behaves like Show(), but it will also instruct the manager to 
  // bring up the first page of the panel to the front.
  // IsVisible() will check if the pages of this panel are visible/shown.
  // Return 1 on success, 0 on error.
  virtual int Show();
  virtual int Raise();
  virtual int IsVisible();

  // Description:
  // Refresh the interface.
  virtual void Update();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState() {};

protected:
  vtkKWUserInterfacePanel();
  ~vtkKWUserInterfacePanel();

  vtkKWUserInterfaceManager *UserInterfaceManager;

  char *Name;

  int Enabled;

private:

  vtkKWUserInterfacePanel(const vtkKWUserInterfacePanel&); // Not implemented
  void operator=(const vtkKWUserInterfacePanel&); // Not Implemented
};

#endif

