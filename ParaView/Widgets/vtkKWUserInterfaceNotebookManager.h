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
// .NAME vtkKWUserInterfaceNotebookManager - a user interface manager.
// .SECTION Description
// This class is used to abstract the way a set of interface "panels" 
// (vtkKWUserInterfacePanel) can be grouped inside a widget. As such, it is a 
// concrete implementation of a vtkKWUserInterfaceManager. It uses a notebook
// under the hood and delivers a notebook's page when one of its managed panels
// request a "page" (i.e. a section within a panel). Within the notebook, each
// page will be associated to a tag corresponding to its panel's ID. This allows
// panels to be shown once at a time, or grouped, or displayed using more 
// advanced combination (like most recently used pages among all panels, pinned 
// pages, etc.).
// This class is not a widget, the notebook is. Besides packing the notebook, 
// you will just have to set each panel's UserInterfaceManager ivar to point
// to this manager, and the rest should be taken care of (i.e. you do not 
// need to manually add a panel to a manager, or manually request a page from
// the manager, it should be done through the panel's API).
// .SECTION See Also
// vtkKWUserInterfaceManager vtkKWUserInterfacePanel

#ifndef __vtkKWUserInterfaceNotebookManager_h
#define __vtkKWUserInterfaceNotebookManager_h

#include "vtkKWUserInterfaceManager.h"

class vtkKWApplication;
class vtkKWIcon;
class vtkKWNotebook;
class vtkKWUserInterfacePanel;
class vtkKWWidget;

class VTK_EXPORT vtkKWUserInterfaceNotebookManager : public vtkKWUserInterfaceManager
{
public:
  static vtkKWUserInterfaceNotebookManager* New();
  vtkTypeRevisionMacro(vtkKWUserInterfaceNotebookManager,vtkKWUserInterfaceManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the user interface manager's notebook. This has to be done before
  // Create() is called (i.e. the sooner, the better), and can be done only
  // once.
  virtual void SetNotebook(vtkKWNotebook*);
  vtkGetObjectMacro(Notebook, vtkKWNotebook);
  
  // Description:
  // Create the manager widget (i.e. the widget that will group and display
  // all user interface panels). A notebook must be associated to the manager
  // before it is created.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Instruct the manager to reserve a page for a given panel. 
  // In this concrete implementation, this adds a page to the notebook, 
  // and sets the page tag to be the panel's ID.
  // Note that you should use the panel's own API to add a page to a panel: this
  // will automatically call this method with the proper panel parameter 
  // (see vtkKWUserInterfacePanel::AddPage()).
  // Return a unique positive ID, or < 0 on error.
  virtual int AddPage(vtkKWUserInterfacePanel *panel, 
                      const char *title, 
                      const char *balloon = 0, 
                      vtkKWIcon *icon = 0);

  // Description:
  // Retrieve the widget corresponding to a given page reserved by the manager.
  // This can be done through the unique page ID, or using a panel and the
  // page title. The user UI components should be inserted into this widget.
  // In this concrete implementation, this returns the inner frame of a
  // notebook's page.
  // Note that you should use the panel's own API to get a page widget: this
  // will automatically call this method with the proper ID or panel parameter
  // (see vtkKWUserInterfacePanel::GetPageWidget()).
  // Return NULL on error.
  virtual vtkKWWidget* GetPageWidget(int id);
  virtual vtkKWWidget* GetPageWidget(vtkKWUserInterfacePanel *panel, 
                                     const char *title);

  // Description:
  // Retrieve the parent widget of the pages associated to a panel. It is
  // the unique widget that is common to all pages in the chain of parents.
  // Note that you should use the panel's own API to get the page parent: this
  // will automatically call this method with the proper panel parameter
  // (see vtkKWUserInterfacePanel::GetPagesParentWidget()).
  virtual vtkKWWidget *GetPagesParentWidget(vtkKWUserInterfacePanel *panel);

  // Description:
  // Raise a page reserved by the manager. This can be done through the unique 
  // page ID, or using a panel and the page title.
  // In this concrete implementation, this raises a notebook's page.
  // Note that you should use the panel's own API to raise a page: this
  // will automatically call this method with the proper ID or panel parameter
  // (see vtkKWUserInterfacePanel::RaisePage()).
  // Note that if the panel corresponding to the page to raise has not been
  // created yet, it will be created automatically by calling the panel's 
  // Create() method (see vtkKWUserInterfacePanel::Create()) ; this allows the
  // creation of the panel to be delayed until it is really needed.
  virtual void RaisePage(int id);
  virtual void RaisePage(vtkKWUserInterfacePanel *panel, 
                         const char *title);
  
  // Description:
  // Show a panel. It will make sure the pages reserved by the manager for 
  // this panel are shown.
  // In this concrete implementation, this shows all notebook's pages belonging
  // to this panel.
  // RaisePanel() behaves like ShowPanel(), but it will also try to bring
  // up the first page of the panel to the front (i.e., "select" it).
  // Note that you should use the panel's own API to show a panel: this
  // will automatically call this method with the proper panel parameter
  // (see vtkKWUserInterfacePanel::Show()).
  // Note that if the panel has not been created yet, it will be created 
  // automatically by calling the panel's Create() method (see 
  // vtkKWUserInterfacePanel::Create()) ; this allows the creation of the 
  // panel to be delayed until it is really needed.
  // Return 1 on success, 0 on error.
  virtual int ShowPanel(vtkKWUserInterfacePanel *panel);
  virtual int RaisePanel(vtkKWUserInterfacePanel *panel);
  
  // Description:
  // Update a panel according to the manager settings (i.e., it just performs 
  // manager-specific changes on the panel). Note that it does not call the
  // panel's Update() method, on the opposite the panel's Update() will call this
  // method if the panel has a UIM set.
  virtual void UpdatePanel(vtkKWUserInterfacePanel *panel);

  // Description:
  // Enable/disable Drag and Drop.
  virtual void SetEnableDragAndDrop(int);
  vtkBooleanMacro(EnableDragAndDrop, int);
  vtkGetMacro(EnableDragAndDrop, int);

  // Description:
  // Drag and Drop callback
  virtual void DragAndDropEndCallback(
    int x, int y, 
    vtkKWWidget *widget, vtkKWWidget *anchor, vtkKWWidget *target);

  // Description:
  // Write the list of visible pages to a stream, parse the same kind of
  // list from a stream (and show the pages). Pinned status is saved too.
  virtual void WriteVisiblePagesString(ostream &os);
  virtual void ParseVisiblePagesString(istream &is);

protected:
  vtkKWUserInterfaceNotebookManager();
  ~vtkKWUserInterfaceNotebookManager();

  // Description:
  // Remove the widgets of all pages belonging to a panel. It is called
  // by RemovePanel().
  // In this concrete implementation, this will remove all notebook's pages 
  // belonging to this panel.
  // Return 1 on success, 0 on error.
  virtual int RemovePageWidgets(vtkKWUserInterfacePanel *panel);
  
  vtkKWNotebook *Notebook;

  // Description:
  // Update Drag And Drop bindings
  virtual void UpdatePanelDragAndDrop(vtkKWUserInterfacePanel *panel);
  int EnableDragAndDrop;

private:

  vtkKWUserInterfaceNotebookManager(const vtkKWUserInterfaceNotebookManager&); // Not implemented
  void operator=(const vtkKWUserInterfaceNotebookManager&); // Not Implemented
};

#endif

