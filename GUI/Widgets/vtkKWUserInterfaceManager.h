/*=========================================================================

  Module:    vtkKWUserInterfaceManager.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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
class vtkKWUserInterfaceManagerInternals;

class VTK_EXPORT vtkKWUserInterfaceManager : public vtkKWObject
{
public:
  vtkTypeRevisionMacro(vtkKWUserInterfaceManager,vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the manager widget (i.e. the widget that will group and display
  // all user interface panels).
  virtual void Create(vtkKWApplication *app);
  virtual int IsCreated();

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
  // Get the number of panel
  virtual int GetNumberOfPanels();

  // Description:
  // Convenience method to get the panel from its name or ID, from a page
  // ID (return the ID of the panel that holds that page), or the nth panel
  virtual vtkKWUserInterfacePanel* GetPanel(const char *panel_name);
  virtual vtkKWUserInterfacePanel* GetPanel(int id);
  virtual vtkKWUserInterfacePanel* GetPanelFromPageId(int id) = 0;
  virtual vtkKWUserInterfacePanel* GetNthPanel(int rank);

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
  // Show/Hide a panel. It will make sure the pages reserved by the manager
  // for this panel are shown/hidden.
  // RaisePanel() behaves like ShowPanel(), but it will also try to bring
  // up the first page of the panel to the front (i.e., "select" it).
  // IsPanelVisible() checks if the pages of the panel are visible/shown.
  // Note that you should use the panel's own API to show/raise a panel: this
  // will automatically call this method with the proper panel parameter
  // (see vtkKWUserInterfacePanel::Show/Raise()).
  // Return 1 on success, 0 on error.
  virtual int ShowPanel(vtkKWUserInterfacePanel *panel) = 0;
  virtual int HidePanel(vtkKWUserInterfacePanel *panel) = 0;
  virtual int IsPanelVisible(vtkKWUserInterfacePanel *panel) = 0;
  virtual int RaisePanel(vtkKWUserInterfacePanel *panel) 
    { return this->ShowPanel(panel); };

  // Description:
  // Convenience method to show/hide all panels.
  virtual void ShowAllPanels();
  virtual void HideAllPanels();

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
 
  class PanelSlot
  {
  public:
    int Id;
    vtkKWUserInterfacePanel *Panel;
  };

  // PIMPL Encapsulation for STL containers

  vtkKWUserInterfaceManagerInternals *Internals;
  friend class vtkKWUserInterfaceManagerInternals;

  // Helper methods
  // Get a panel slot given a panel or an id, check if the manager has a given
  // panel, get a panel id given a panel, etc.

  PanelSlot* GetPanelSlot(vtkKWUserInterfacePanel *panel);
  PanelSlot* GetPanelSlot(int id);
  PanelSlot* GetPanelSlot(const char *panel_name);
  PanelSlot* GetNthPanelSlot(int rank);
  int HasPanel(vtkKWUserInterfacePanel *panel);
  int GetPanelId(vtkKWUserInterfacePanel *panel);

  //ETX

private:

  vtkKWUserInterfaceManager(const vtkKWUserInterfaceManager&); // Not implemented
  void operator=(const vtkKWUserInterfaceManager&); // Not Implemented
};

#endif

