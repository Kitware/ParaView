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
// .NAME vtkKWUserInterfaceManager - a user interface manager.
// .SECTION Description
// This class is used to abstract the way a set of interface "panels" 
// (vtkKWUserInterfacePanel) can be grouped inside a widget. 
// For example, if a concrete implementation of that class
// uses a notebook as its underlying widget, then it will deliver a notebook's 
// page when one of its managed panels request a "page" (i.e. a section 
// within a panel). If another concrete implementation chooses for
// a flat GUI for example, then it will likely return frames as pages and 
// pack them on top of each other.
// This class is not a widget. Concrete implementation of this class will
// provide an access point to a widget into which the manager will organize
// its panels (a notebook, a frame, etc.). 
// Besides packing this widget, you will just have to set each panel's
// UserInterfaceManager ivar to point to this manager, and the rest should 
// be taken care of (i.e. you do not need to manually add a panel to a manager,
// or manually request a page from the manager, it should be done through the 
// panel's API).
// .SECTION See Also
// vtkKWUserInterfaceNotebookManager vtkKWUserInterfacePanel

#ifndef __vtkKWUserInterfaceManager_h
#define __vtkKWUserInterfaceManager_h

#include "vtkKWObject.h"

class vtkKWApplication;
class vtkKWIcon;
class vtkKWWidget;
class vtkKWUserInterfacePanel;

//BTX
template<class DataType> class vtkLinkedList;
template<class DataType> class vtkLinkedListIterator;
//ETX

class VTK_EXPORT vtkKWUserInterfaceManager : public vtkKWObject
{
public:
  vtkTypeRevisionMacro(vtkKWUserInterfaceManager,vtkKWObject);

  // Description:
  // Create the manager widget (i.e. the widget that will group and display
  // all user interface panels).
  virtual void Create(vtkKWApplication *app);
  virtual int IsCreated() { return (this->Application != 0); }

  // Description:
  // Enable/Disable this interface. This propagates SetEnabled() calls to all
  // panels.
  virtual void SetEnabled(int);
  virtual void UpdateEnableState();

  // Description:
  // Iterate over all panels and call Update() for each one. This will refresh
  // the panels (provided that Update() has been reimplemented).
  virtual void Update();

  // Description:
  // Add a panel to the manager.
  // Note that you most probably do not need to call this method, since setting
  // a panel's UserInterfaceManager ivar will add the panel automatically 
  // (see vtkKWUserInterfacePanel::SetUserInterfaceManager()).
  // Return a unique positive ID corresponding to that panel, or < 0 on error.
  virtual int AddPanel(vtkKWUserInterfacePanel *panel);

  // Description:
  // Convenience method to get the panel from its name or ID
  vtkKWUserInterfacePanel* GetPanel(const char *panel_name);
  vtkKWUserInterfacePanel* GetPanel(int id);

  // Description:
  // Remove a panel from the manager.
  // Note that you most probably do not need to call this method, since setting
  // a panel's UserInterfaceManager ivar to NULL will remove the panel 
  // automatically (this is done in the panel's destructor).
  // Return 1 on success, 0 on error.
  virtual int RemovePanel(vtkKWUserInterfacePanel *panel);

  // Description:
  // Instruct the manager to reserve a page for a given panel.
  // Note that you should use the panel's own API to add a page to a panel: 
  // this will automatically call this method with the proper panel parameter 
  // (see vtkKWUserInterfacePanel::AddPage()).
  // Return a unique positive ID, or < 0 on error.
  virtual int AddPage(vtkKWUserInterfacePanel *panel, 
                      const char *title, 
                      const char *balloon = 0, 
                      vtkKWIcon *icon = 0) = 0;

  // Description:
  // Retrieve the widget corresponding to a given page reserved by the manager.
  // This can be done through the unique page ID, or using a panel and the
  // page title. The user UI components should be inserted into this widget.
  // Note that you should use the panel's own API to get a page widget: this
  // will automatically call this method with the proper ID or panel parameter
  // (see vtkKWUserInterfacePanel::GetPageWidget()).
  // Return NULL on error.
  virtual vtkKWWidget* GetPageWidget(int id) = 0;
  virtual vtkKWWidget* GetPageWidget(vtkKWUserInterfacePanel *panel, 
                                     const char *title) = 0;
                      
  // Description:
  // Retrieve the parent widget of the pages associated to a panel. It is
  // the unique widget that is common to all pages in the chain of parents.
  // Note that you should use the panel's own API to get the page parent: this
  // will automatically call this method with the proper panel parameter
  // (see vtkKWUserInterfacePanel::GetPagesParentWidget()).
  virtual vtkKWWidget *GetPagesParentWidget(vtkKWUserInterfacePanel *panel)= 0;

  // Description:
  // Raise a page reserved by the manager. This can be done through the unique 
  // page ID, or using a panel and the page title.
  // Note that you should use the panel's own API to raise a page: this
  // will automatically call this method with the proper ID or panel parameter
  // (see vtkKWUserInterfacePanel::RaisePage()).
  virtual void RaisePage(int id) = 0;
  virtual void RaisePage(vtkKWUserInterfacePanel *panel, 
                         const char *title) = 0;

  // Description:
  // Show a panel. It will make sure the pages reserved by the manager for 
  // this panel are shown.
  // RaisePanel() behaves like ShowPanel(), but it will also try to bring
  // up the first page of the panel to the front (i.e., "select" it).
  // Note that you should use the panel's own API to show/raise a panel: this
  // will automatically call this method with the proper panel parameter
  // (see vtkKWUserInterfacePanel::Show/Raise()).
  // Return 1 on success, 0 on error.
  virtual int ShowPanel(vtkKWUserInterfacePanel *panel) = 0;
  virtual int RaisePanel(vtkKWUserInterfacePanel *panel) 
    { return this->ShowPanel(panel); };

  // Description:
  // Update a panel according to the manager settings (i.e., it just performs 
  // manager-specific changes on the panel). Note that it does not call the
  // panel's Update() method, on the opposite the panel's Update() will call
  // this method if the panel has a UIM set.
  virtual void UpdatePanel(vtkKWUserInterfacePanel *panel) = 0;

protected:
  vtkKWUserInterfaceManager();
  ~vtkKWUserInterfaceManager();

  // Description:
  // Remove the widgets of all pages belonging to a panel. It is called
  // by RemovePanel() and should be overloaded if the concrete implementation
  // of the manager needs to unmap/unpack widgets before everything is deleted.
  // Return 1 on success, 0 on error.
  virtual int RemovePageWidgets(vtkKWUserInterfacePanel *) 
    { return 1; };

  int IdCounter;

  //BTX

  // A panel slot associate a panel to a unique Id
  // No, I don't want to use a map between those two, for the following reasons:
  // a), we might need more information in the future, b) a map 
  // Register/Unregister pointers if they are pointers to VTK objects.
 
  class PanelSlot
  {
  public:
    int Id;
    vtkKWUserInterfacePanel *Panel;
  };

  typedef vtkLinkedList<PanelSlot*> PanelsContainer;
  typedef vtkLinkedListIterator<PanelSlot*> PanelsContainerIterator;
  PanelsContainer *Panels;

  // Helper methods
  // Get a panel slot given a panel or an id, check if the manager has a given
  // panel, get a panel id given a panel, etc.

  PanelSlot* GetPanelSlot(vtkKWUserInterfacePanel *panel);
  PanelSlot* GetPanelSlot(int id);
  PanelSlot* GetPanelSlot(const char *panel_name);
  int HasPanel(vtkKWUserInterfacePanel *panel);
  int GetPanelId(vtkKWUserInterfacePanel *panel);

  //ETX

private:

  vtkKWUserInterfaceManager(const vtkKWUserInterfaceManager&); // Not implemented
  void operator=(const vtkKWUserInterfaceManager&); // Not Implemented
};

#endif

