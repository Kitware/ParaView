/*=========================================================================

  Module:    vtkKWUserInterfaceManagerDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWUserInterfaceManagerDialog - a user interface manager.
// .SECTION Description
// This class is used to abstract the way a set of interface "panels" 
// (vtkKWUserInterfacePanel) can be grouped inside a widget. As such, it is a 
// concrete implementation of a vtkKWUserInterfaceManager. It uses a dialog
// under the hood to display all pages in a "Preferences" dialog style: the
// dialog is split into two parts: on the left, a tree with entries
// corresponding to the name of specific UI elements found in the panels. 
// If an entry is selected, the corresponding UI element is display on the
// right side of the dialog. This allows a lot of small UI elements to be
// accessed pretty easily while keeping the size of the whole dialog small.
// For each panel, this class creates an entry in the tree using the
// panel name. For each page in the panel, it creates a sub-entry (leaf)
// under the panel name entry, using the page name. Then for each UI elements
// in that page, it looks for instances of vtkKWFrameWithLabel. This is the
// only constraint put on the panels, other than that, the panels
// (vtkKWUserInterfacePanel) can be created as usual, and managed by any
// subclass of vtkKWUserInterfaceManager. This is not too big a constraint
// since most panels are built that way. For a concrete example of such
// a panel, check vtkKWApplicationSettingsInterface.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWUserInterfaceManager vtkKWUserInterfacePanel vtkKWApplicationSettingsInterface

#ifndef __vtkKWUserInterfaceManagerDialog_h
#define __vtkKWUserInterfaceManagerDialog_h

#include "vtkKWUserInterfaceManager.h"

class vtkKWIcon;
class vtkKWNotebook;
class vtkKWPushButton;
class vtkKWSeparator;
class vtkKWSplitFrame;
class vtkKWTopLevel;
class vtkKWTreeWithScrollbars;
class vtkKWUserInterfaceManagerDialogInternals;
class vtkKWUserInterfacePanel;
class vtkKWWidget;

class KWWidgets_EXPORT vtkKWUserInterfaceManagerDialog : public vtkKWUserInterfaceManager
{
public:
  static vtkKWUserInterfaceManagerDialog* New();
  vtkTypeRevisionMacro(vtkKWUserInterfaceManagerDialog,vtkKWUserInterfaceManager);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the manager widget (i.e. the widget that will group and display
  // all user interface panels). A notebook must be associated to the manager
  // before it is created.
  virtual void Create();

  // Description:
  // Set the panel node visibility. If On, each panel will show up as
  // a node in the tree, acting as a parent to the page nodes or the section
  // nodes. Defaults to Off to avoid too much clutter in the tree.
  virtual void SetPanelNodeVisibility(int);
  vtkGetMacro(PanelNodeVisibility, int);
  vtkBooleanMacro(PanelNodeVisibility, int);

  // Description:
  // Set the page node visibility. If On, each page will show up as
  // a node in the tree, acting as a parent to the section nodes.
  // Default to On. Since sections can have the same name within different
  // pages, it is advised to leave it On. 
  virtual void SetPageNodeVisibility(int);
  vtkGetMacro(PageNodeVisibility, int);
  vtkBooleanMacro(PageNodeVisibility, int);

  // Description:
  // Access to the dialog/toplevel
  // Can be used to change its title, and master window
  vtkGetObjectMacro(TopLevel, vtkKWTopLevel);

  // Description:
  // Get the application instance for this object.
  // Override the superclass to try to retrieve the toplevel's application
  // if it was not set already.
  virtual vtkKWApplication* GetApplication();

  // Description:
  // Raise a specific section, given a panel, a page id (or page title) 
  // and a section name. If panel is NULL, page_id is < 0, 
  // page_title is NULL or empty, section is NULL or empty, then any of these
  // parameters will be ignored and the first matching section will be picked.
  virtual void RaiseSection(int page_id, 
                            const char *section);
  virtual void RaiseSection(vtkKWUserInterfacePanel *panel, 
                            const char *page_title, 
                            const char *section);

  // Description:
  // Instruct the manager to reserve or remove a page for a given panel.
  // In this concrete implementation, this adds or removes a page to the 
  // notebook, and sets the page tag to be the panel's ID.
  // Note that you should use the panel's own API to add a page to a panel: 
  // this will automatically call this method with the proper panel parameter 
  // (see vtkKWUserInterfacePanel::AddPage() and 
  // vtkKWUserInterfacePanel::RemovePage()).
  // Return a unique positive ID for the page that was reserved/removed,
  // or < 0 on error.
  virtual int AddPage(vtkKWUserInterfacePanel *panel, 
                      const char *title, 
                      const char *balloon = 0, 
                      vtkKWIcon *icon = 0);
  virtual int RemovePage(vtkKWUserInterfacePanel *panel, 
                         const char *title);

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
  // Show/Hide a panel. It will make sure the pages reserved by the manager
  // for this panel are shown/hidden.
  // In this concrete implementation, this shows/hides all notebook's pages
  // belonging to this panel.
  // RaisePanel() behaves like ShowPanel(), but it will also try to bring
  // up the first page of the panel to the front (i.e., "select" it).
  // IsPanelVisible() checks if the pages of the panel are visible/shown.
  // Note that you should use the panel's own API to show a panel: this
  // will automatically call this method with the proper panel parameter
  // (see vtkKWUserInterfacePanel::Show()).
  // Note that if the panel has not been created yet, it will be created 
  // automatically by calling the panel's Create() method (see 
  // vtkKWUserInterfacePanel::Create()) ; this allows the creation of the 
  // panel to be delayed until it is really needed.
  // Return 1 on success, 0 on error.
  virtual int ShowPanel(vtkKWUserInterfacePanel *panel);
  virtual int HidePanel(vtkKWUserInterfacePanel *panel);
  virtual int IsPanelVisible(vtkKWUserInterfacePanel *panel);
  
  // Description:
  // Get the panel from a page ID (return the ID of the panel that holds
  // that page).
  virtual vtkKWUserInterfacePanel* GetPanelFromPageId(int page_id);

  // Description:
  // Set/Get the vertical scrollbar visibility of the tree (off by default)
  virtual void SetVerticalScrollbarVisibility(int val);
  virtual int GetVerticalScrollbarVisibility();
  vtkBooleanMacro(VerticalScrollbarVisibility, int);

  // Description:
  // Callbacks. Internal, do not use.
  virtual void SelectionChangedCallback();

protected:
  vtkKWUserInterfaceManagerDialog();
  ~vtkKWUserInterfaceManagerDialog();

  // Description:
  // Remove the widgets of all pages belonging to a panel. It is called
  // by RemovePanel().
  // In this concrete implementation, this will remove all notebook's pages 
  // belonging to this panel.
  // Return 1 on success, 0 on error.
  virtual int RemovePageWidgets(vtkKWUserInterfacePanel *panel);
  
  vtkKWNotebook           *Notebook;
  vtkKWTopLevel           *TopLevel;
  vtkKWSplitFrame         *SplitFrame;
  vtkKWTreeWithScrollbars *Tree;
  vtkKWPushButton         *CloseButton;
  vtkKWSeparator          *Separator;

  // PIMPL Encapsulation for STL containers
  //BTX
  vtkKWUserInterfaceManagerDialogInternals *Internals;
  //ETX

  virtual void PopulateTree();
  virtual int ShowSelectedNodeSection();
  virtual int CreateAllPanels();

  int PanelNodeVisibility;
  int PageNodeVisibility;

  // Description:
  // This method is (and should be) called each time the number of panels
  // changes (for example, after AddPanel() / RemovePanel())
  virtual void NumberOfPanelsChanged();

  int GetWidgetLocation(
    const char *widget, vtkKWUserInterfacePanel **panel, int *page_id);

private:

  vtkKWUserInterfaceManagerDialog(const vtkKWUserInterfaceManagerDialog&); // Not implemented
  void operator=(const vtkKWUserInterfaceManagerDialog&); // Not Implemented
};

#endif

