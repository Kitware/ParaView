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
// page will be associated to a tag corresponding to its panel's ID. This 
// allows panels to be shown once at a time, or grouped, or displayed using 
// more advanced combination (like most recently used pages among all panels, 
// pinned pages, etc.).
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
  // Note that you should use the panel's own API to add a page to a panel: 
  // this will automatically call this method with the proper panel parameter 
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
  // Show/Hide a panel. It will make sure the pages reserved by the manager
  // for this panel are shown/hidden.
  // In this concrete implementation, this shows/hides all notebook's pages
  // belonging to this panel.
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
  virtual int HidePanel(vtkKWUserInterfacePanel *panel);
  virtual int RaisePanel(vtkKWUserInterfacePanel *panel);
  
  // Description:
  // Update a panel according to the manager settings (i.e., it just performs 
  // manager-specific changes on the panel). Note that it does not call the
  // panel's Update() method, on the opposite the panel's Update() will call 
  // this method if the panel has a UIM set.
  virtual void UpdatePanel(vtkKWUserInterfacePanel *panel);

  // Description:
  // Convenience method to get the panel from a page
  // ID (return the ID of the panel that holds that page).
  virtual vtkKWUserInterfacePanel* GetPanelFromPageId(int page_id);

  // Description:
  // Enable/disable Drag and Drop. If enabled, elements of the user interface
  // can be drag&drop within the same panel, or between different panels.
  virtual void SetEnableDragAndDrop(int);
  vtkBooleanMacro(EnableDragAndDrop, int);
  vtkGetMacro(EnableDragAndDrop, int);

  // Description:
  // Drag and Drop callback
  virtual void DragAndDropEndCallback(
    int x, int y, 
    vtkKWWidget *widget, vtkKWWidget *anchor, vtkKWWidget *target);

  // Description:
  // Get the number of Drag&Drop entries so far.
  // Delete all Drag&Drop entries.
  virtual int GetNumberOfDragAndDropEntries();
  virtual int DeleteAllDragAndDropEntries();

  // Description:
  // Convenience function used to serialize/save/restore Drag&Drop entries to
  // a text file. 
  // GetDragAndDropEntry() can be used to get a Drag&Drop entry parameters 
  // as plain text string. 
  // DragAndDropWidget() will perform a Drag&Drop given parameters similar
  // to those acquired through GetDragAndDropEntry().
  virtual int GetDragAndDropEntry(
    int idx, 
    ostream &widget_label, 
    ostream &from_panel_name, 
    ostream &from_page_title, 
    ostream &from_after_widget_label, 
    ostream &to_panel_name, 
    ostream &to_page_title,
    ostream &to_after_widget_label);
  virtual int DragAndDropWidget(
    const char *widget_label, 
    const char *from_panel_name, 
    const char *from_page_title, 
    const char *from_after_widget_label,
    const char *to_panel_name, 
    const char *to_page_title,
    const char *to_after_widget_label);

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

  //BTX

  // Description:
  // Check if a given widget can be Drag&Dropped given our framework.
  // At the moment, only labeled frame can be D&D. If **anchor is not NULL,
  // it will be assigned the widget D&D anchor (i.e. the internal part of
  // the widget that is actually used to grab the widget).
  // Return 1 if can be D&D, 0 otherwise.
  virtual int CanWidgetBeDragAndDropped(
    vtkKWWidget *widget, vtkKWWidget **anchor = 0);

  // Description:
  // Assuming that the widget can be Drag&Dropped given our framework, 
  // return a label that will be used to identify it. This is mostly used to
  // serialize a D&D event to a text string/file.
  virtual char* GetDragAndDropWidgetLabel(vtkKWWidget *widget);

  // A Widget location. 
  // Store both the page the widget is packed in, and the widget it is 
  // packed after (if any).

  class WidgetLocation
  {
  public:
    WidgetLocation();

    int PageId;
    vtkKWWidget *AfterWidget;
  };

  // Description:
  // Get the location of a widget.
  virtual int GetDragAndDropWidgetLocation(
    vtkKWWidget *widget, WidgetLocation *loc);

  // Description:
  // Get a D&D widget given its label (as returned by 
  // GetDragAndDropWidgetLabel()) and a hint about its location.
  virtual vtkKWWidget* GetDragAndDropWidgetFromLabelAndLocation(
    const char *widget_label, WidgetLocation *loc_hint);

  // A D&D entry. 
  // Store the widget source and target location.

  class DragAndDropEntry
  {
  public:
    DragAndDropEntry();

    vtkKWWidget *Widget;
    WidgetLocation FromLocation;
    WidgetLocation ToLocation;
  };

  // List of D&D entries

  typedef vtkLinkedList<DragAndDropEntry*> DragAndDropEntriesContainer;
  typedef vtkLinkedListIterator<DragAndDropEntry*>
  DragAndDropEntriesContainerIterator;
  DragAndDropEntriesContainer *DragAndDropEntries;

  // Description:
  // Get the last D&D entry that was added for a given widget
  DragAndDropEntry* GetLastDragAndDropEntry(vtkKWWidget *Widget);

  // Description:
  // Add a D&D entry to the list of entries, given a widget and its
  // target location (its current/source location will be computed 
  // automatically)
  int AddDragAndDropEntry(vtkKWWidget *Widget, WidgetLocation *to_loc);

  // Description:
  // Perform the actual D&D given a widget and its target location.
  // It will call AddDragAndDropEntry() and pack the widget to its new
  // location
  virtual int DragAndDropWidget(vtkKWWidget *widget, WidgetLocation *to_loc);

  //ETX

private:

  vtkKWUserInterfaceNotebookManager(const vtkKWUserInterfaceNotebookManager&); // Not implemented
  void operator=(const vtkKWUserInterfaceNotebookManager&); // Not Implemented
};

#endif

