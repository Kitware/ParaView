/*=========================================================================

  Module:    vtkKWUserInterfaceNotebookManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWUserInterfaceNotebookManager.h"

#include "vtkCollectionIterator.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWFrameLabeled.h"
#include "vtkKWNotebook.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWidgetCollection.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWUserInterfaceNotebookManager);
vtkCxxRevisionMacro(vtkKWUserInterfaceNotebookManager, "1.28");

int vtkKWUserInterfaceNotebookManagerCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::vtkKWUserInterfaceNotebookManager()
{
  // The parent class (vtkKWUserInterfaceManager) initializes IdCounter so that
  // panel IDs starts at 0. In this class, a panel ID will map to a notebook 
  // tag (a tag is an integer associated to each notebook page). The default 
  // tag, if not specified, is 0. By starting at 1 we will avoid mixing managed
  // and unmanaged pages (unmanaged pages are directly added to the notebook 
  // without going through the manager).

  this->IdCounter = 1;
  this->Notebook = NULL;
  this->EnableDragAndDrop = 0;
  this->LockDragAndDropEntries = 0;

  this->DragAndDropEntries = 
    vtkKWUserInterfaceNotebookManager::DragAndDropEntriesContainer::New();
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::~vtkKWUserInterfaceNotebookManager()
{
  this->SetNotebook(NULL);

  // Delete all d&d entries

  this->DeleteAllDragAndDropEntries();

  // Delete the container

  this->DragAndDropEntries->Delete();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::SetNotebook(vtkKWNotebook *_arg)
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this 
                << "): setting Notebook to " << _arg);

  if (this->Notebook == _arg)
    {
    return;
    }

  if (this->IsCreated() && _arg)
    {
    vtkErrorMacro("The notebook cannot be changed once the manager "
                  "has been created.");
    return;
    }

  if (this->Notebook != NULL) 
    { 
    this->Notebook->UnRegister(this); 
    }

  this->Notebook = _arg; 

  if (this->Notebook != NULL) 
    { 
    this->Notebook->Register(this); 
    } 

  this->Modified(); 
} 

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created");
    return;
    }

  // We need a notebook

  if (!this->Notebook)
    {
    vtkErrorMacro("A notebook must be associated to this manager before it "
                  " is created");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create(app);
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::AddPage(
  vtkKWUserInterfacePanel *panel, 
  const char *title, 
  const char *balloon, 
  vtkKWIcon *icon)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not add a page if the manager has not been created.");
    return -1;
    }
 
  if (!panel)
    {
    vtkErrorMacro("Can not add a page to a NULL panel.");
    return -1;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not add a page to a panel that is not in the manager.");
    return -1;
    }

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to add a page to.");
    return -1;
    }

  // Use the panel id as a tag in the notebook, so that the pages belonging
  // to this panel will correspond to notebook pages sharing a same tag.

  return this->Notebook->AddPage(title, balloon, icon, tag);
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWUserInterfaceNotebookManager::GetPageWidget(int id)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not query a page if the manager has not been created.");
    return NULL;
    }

  // Since each page has a unique id, whatever the panel it belongs to, just 
  // retrieve the frame of the corresponding notebook page.

  return this->Notebook->GetFrame(id);
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWUserInterfaceNotebookManager::GetPageWidget(
  vtkKWUserInterfacePanel *panel, 
  const char *title)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not query a page if the manager has not been created.");
    return NULL;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not query a page from a NULL panel.");
    return NULL;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not query a page from a panel that is not "
                  "in the manager.");
    return NULL;
    }

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to query a page.");
    return NULL;
    }

  // Access the notebook page that has this specific title among the notebook 
  // pages that share the same tag (i.e. among the pages that belong to the 
  // same panel). This allow pages from different panels to have the same 
  // title.

  return this->Notebook->GetFrame(title, tag);
}

//----------------------------------------------------------------------------
vtkKWWidget* vtkKWUserInterfaceNotebookManager::GetPagesParentWidget(
  vtkKWUserInterfacePanel *vtkNotUsed(panel))
{
  // Here we probably need this->Notebook->Body but it's not a public member

  return this->Notebook;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::RaisePage(int id)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not raise a page if the manager has not been created.");
    return;
    }

  int tag = this->Notebook->GetPageTag(id);
  vtkKWUserInterfacePanel *panel = this->GetPanel(tag);
  if (!panel)
    {
    vtkErrorMacro("Can not raise a page from a NULL panel.");
    return;
    }

  // Make sure the panel is shown (and created)

  this->ShowPanel(panel);
  
  // Since each page has a unique id, whatever the panel it belongs to, just 
  // raise the corresponding notebook page.

  this->Notebook->Raise(id);
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::RaisePage(
  vtkKWUserInterfacePanel *panel, 
  const char *title)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not query a page if the manager has not been created.");
    return;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not raise a page from a NULL panel.");
    return;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not raise a page from a panel that is not "
                  "in the manager.");
    return;
    }

  // Make sure the panel is shown (and created)

  this->ShowPanel(panel);
  
  // Raise the notebook page that has this specific title among the notebook 
  // pages that share the same tag (i.e. among the pages that belong to the 
  // same panel). This allow pages from different panels to have the same 
  // title.

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to raise a page.");
    return;
    }

  this->Notebook->Raise(title, tag);
}


//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::ShowPanel(
  vtkKWUserInterfacePanel *panel)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not show pages if the manager has not been created.");
    return 0;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not show the pages from a NULL panel.");
    return 0;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not show the pages from a panel that is not "
                  "in the manager.");
    return 0;
    }

  // As a convenience, if the panel that this page was created for has 
  // not been created yet, it is created now. This allow the GUI creation to 
  // be delayed until it is really needed.

  if (!panel->IsCreated())
    {
    panel->Create(this->GetApplication());
    }

  // Show the pages that share the same tag (i.e. the pages that belong to the 
  // same panel).

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to show its pages.");
    return 0;
    }

  // If the notebook is maintaining most recent pages, we have to show
  // the pages belonging to the same group starting from the last one, so
  // that the most recent one will be the first page in that group (i.e.
  // the last shown)

  if (this->Notebook->GetShowOnlyMostRecentPages())
    {
    this->Notebook->ShowPagesMatchingTagReverse(tag);
    }
  else
    {
    this->Notebook->ShowPagesMatchingTag(tag);
    }

  // If there were pages matching that tag, but we end up with *no* pages
  // visible for that tag, then we failed (maybe because of the notebook
  // constraints, the number of pages already pinned, etc).

  if (this->Notebook->GetNumberOfPagesMatchingTag(tag) && 
      !this->Notebook->GetNumberOfVisiblePagesMatchingTag(tag))
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::HidePanel(
  vtkKWUserInterfacePanel *panel)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not hide pages if the manager has not been created.");
    return 0;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not hide the pages from a NULL panel.");
    return 0;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not hide the pages from a panel that is not "
                  "in the manager.");
    return 0;
    }

  // Hide the pages that share the same tag (i.e. the pages that belong to the 
  // same panel).

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to hide its pages.");
    return 0;
    }

  this->Notebook->UnpinPagesMatchingTag(tag);
  this->Notebook->HidePagesMatchingTag(tag);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::IsPanelVisible(
  vtkKWUserInterfacePanel *panel)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro(
      "Can not check pages visiblity if the manager has not been created.");
    return 0;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not check the pages visibility from a NULL panel.");
    return 0;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not check the pages visibility from a panel that is "
                  "not in the manager.");
    return 0;
    }

  // Pages share the same tag

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to show its pages.");
    return 0;
    }

  return (this->Notebook->GetNumberOfPagesMatchingTag(tag) ==
          this->Notebook->GetNumberOfVisiblePagesMatchingTag(tag));
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::RaisePanel(
  vtkKWUserInterfacePanel *panel)
{
  // First show the panel

  if (!this->ShowPanel(panel))
    {
    return 0;
    }

  // If the page raised at the moment is part of this panel, then we are 
  // OK already.

  int tag = this->GetPanelId(panel);
  int current_id = this->Notebook->GetRaisedPageId();
  if (current_id && tag == this->Notebook->GetPageTag(current_id))
    {
    return 1;
    }
 
  // Otherwise raise the first page

  this->Notebook->RaiseFirstPageMatchingTag(tag);

  // If there were pages matching that tag, but we end up with the raised
  // page not matching that tag, then we failed (maybe because of the notebook
  // constraints, the number of pages already pinned, etc).

  current_id = this->Notebook->GetRaisedPageId();
  if (current_id && 
      this->Notebook->GetNumberOfPagesMatchingTag(tag) &&
      this->Notebook->GetPageTag(current_id) != tag)
    {
    return 0;
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::RemovePageWidgets(
  vtkKWUserInterfacePanel *panel)
{
  if (!this->IsCreated())
    {
    vtkErrorMacro("Can not remove page widgets if the manager has not " 
                  "been created.");
    return 0;
    }

  if (!panel)
    {
    vtkErrorMacro("Can not remove page widgets from a NULL panel.");
    return 0;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not remove page widgets from a panel that is not "
                  "in the manager.");
    return 0;
    }

  int tag = this->GetPanelId(panel);
  if (tag < 0)
    {
    vtkErrorMacro("Can not access the panel to remove page widgets.");
    return 0;
    }

  // Remove the pages that share the same tag (i.e. the pages that 
  // belong to the same panel).

  this->Notebook->RemovePagesMatchingTag(tag);

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::UpdatePanel(
  vtkKWUserInterfacePanel *panel)
{
  this->UpdatePanelDragAndDrop(panel);
}

//----------------------------------------------------------------------------
vtkKWUserInterfacePanel* 
vtkKWUserInterfaceNotebookManager::GetPanelFromPageId(int page_id)
{
  if (!this->Notebook || !this->Notebook->HasPage(page_id))
    {
    return 0;
    }

  return this->GetPanel(this->Notebook->GetPageTag(page_id));
}

//---------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::SetEnableDragAndDrop(int arg)
{
  if (this->EnableDragAndDrop == arg)
    {
    return;
    }

  this->EnableDragAndDrop = arg;
  this->Modified();

  // Update all panels Drag And Drop bindings

  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *panel_it = 
    this->Panels->NewIterator();

  panel_it->InitTraversal();
  while (!panel_it->IsDoneWithTraversal())
    {
    if (panel_it->GetData(panel_slot) == VTK_OK)
      {
      this->UpdatePanelDragAndDrop(panel_slot->Panel);
      }
    panel_it->GoToNextItem();
    }
  panel_it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::CanWidgetBeDragAndDropped(
  vtkKWWidget *widget, vtkKWWidget **anchor)
{
  // Check if the widget is a labeled frame (vtkKWFrameLabeled). 
  // In that case, the widget can be drag&dropped, and its anchor is 
  // the widget's anchor itself.
  // If we did not find such a frame, check if it is a widget with a single
  // labeled frame child (i.e. a vtkKWWidget that uses a vtkKWFrameLabeled 
  // but did not want to inherit from vtkKWFrameLabeled)
  // In that case, the widget can be drag&dropped, and its anchor is 
  // the internal labeled frame's anchor.

  if (widget)
    {
    vtkKWFrameLabeled *frame = 
      vtkKWFrameLabeled::SafeDownCast(widget);
    if (!frame && widget->GetChildren()->GetNumberOfItems() == 1)
      {
      frame = vtkKWFrameLabeled::SafeDownCast(
        widget->GetChildren()->GetLastKWWidget());
      }
    if (frame)
      {
      if (anchor)
        {
        *anchor = frame->GetDragAndDropAnchor();
        }
      return 1;
      }
    }

  return 0;
}

//----------------------------------------------------------------------------
char* vtkKWUserInterfaceNotebookManager::GetDragAndDropWidgetLabel(
  vtkKWWidget *widget)
{
  // See CanWidgetBeDragAndDropped() above to understand how we pick
  // the right part of the widget.

  if (widget)
    {
    vtkKWFrameLabeled *frame = 
      vtkKWFrameLabeled::SafeDownCast(widget);
    if (!frame && widget->GetChildren()->GetNumberOfItems() == 1)
      {
      frame = vtkKWFrameLabeled::SafeDownCast(
        widget->GetChildren()->GetLastKWWidget());
      }
    if (frame)
      {
      return frame->GetLabel()->GetLabel();
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::UpdatePanelDragAndDrop(
  vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    vtkErrorMacro("Can not update a NULL panel.");
    return;
    }
  
  if (!this->HasPanel(panel))
    {
    vtkErrorMacro("Can not update a panel that is not in the manager.");
    return;
    }

  if (!this->Notebook)
    {
    return;
    }

  // Get the pages parent, and check if there are widgets
  // that can be Drag&Dropped from one page to the other (since they share the 
  // same parent)

  vtkKWWidget *parent = this->GetPagesParentWidget(panel);
  if (!parent)
    {
    return;
    }

  vtkCollectionIterator *it = parent->GetChildren()->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    vtkKWWidget *widget = vtkKWWidget::SafeDownCast(it->GetCurrentObject());
    vtkKWWidget *anchor = 0;

    // Enable/Disable Drag & Drop for that widget, 
    // the notebook is the drop target

    if (this->CanWidgetBeDragAndDropped(widget, &anchor))
      {
      if (this->EnableDragAndDrop)
        {
        if (!widget->HasDragAndDropTarget(this->Notebook))
          {
          widget->EnableDragAndDropOn();
          widget->SetDragAndDropAnchor(anchor);
          widget->AddDragAndDropTarget(this->Notebook);
          widget->SetDragAndDropEndCommand(
            this->Notebook, this, "DragAndDropEndCallback");
          }
        }
      else
        {
        if (widget->HasDragAndDropTarget(this->Notebook))
          {
          widget->RemoveDragAndDropTarget(this->Notebook);
          }
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//---------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::WidgetLocation::WidgetLocation()
{
  this->Empty();
}

//---------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::WidgetLocation::Empty()
{
  this->PageId = -1;
  this->AfterWidget = NULL;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::GetDragAndDropWidgetLocation(
  vtkKWWidget *widget, WidgetLocation *loc)
{
  if (!loc || !this->Notebook || !widget || !widget->IsPacked())
    {
    return 0;
    }

  // Get the page id. The widget is packed -in a notebook page which ID
  // is the page ID (and which tag is the panel ID by the way)
  // Extract the -in parameter from the pack info

  ostrstream in_str;
  if (!vtkKWTkUtilities::GetPackSlaveIn(
        widget->GetApplication()->GetMainInterp(), 
        widget->GetWidgetName(), in_str))
    {
    return 0;
    }

  in_str << ends;
  int page_id = this->Notebook->GetPageIdFromFrameWidgetName(in_str.str());
  in_str.rdbuf()->freeze(0);
  if (page_id < 0)
    {
    return 0;
    }

  loc->Empty();

  loc->PageId = page_id;

  // Query all the slaves in the same page, find the one located before 
  // our widget (if any) so that we can locate the widget among
  // its sibblings.

  ostrstream prev_slave_str;
  ostrstream next_slave_str;

  if (vtkKWTkUtilities::GetPreviousAndNextSlave(
        widget->GetApplication()->GetMainInterp(),
        this->Notebook->GetFrame(page_id)->GetWidgetName(),
        widget->GetWidgetName(),
        prev_slave_str,
        next_slave_str))
    {
    // Get the page's panel, then the panel's page's parent, and check if we
    // can find the previous widget (since they share the same parent)

    prev_slave_str << ends;
    next_slave_str << ends;

    vtkKWUserInterfacePanel *panel = this->GetPanelFromPageId(page_id);
    vtkKWWidget *parent = this->GetPagesParentWidget(panel);
    if (parent)
      {
      if (*prev_slave_str.str())
        {
        loc->AfterWidget = 
          parent->GetChildWidgetWithName(prev_slave_str.str());
        }
      }
    }

  prev_slave_str.rdbuf()->freeze(0);
  next_slave_str.rdbuf()->freeze(0);

  return 1;
}

//----------------------------------------------------------------------------
vtkKWWidget* 
vtkKWUserInterfaceNotebookManager::GetDragAndDropWidgetFromLabelAndLocation(
  const char *widget_label, const WidgetLocation *loc_hint)
{
  if (!widget_label || !loc_hint)
    {
    return NULL;
    }

  // Get the page's panel, then the panel's page's parent, and check if we
  // can find the widget (since they share the same parent)

  vtkKWWidget *page = this->Notebook->GetFrame(loc_hint->PageId);
  vtkKWUserInterfacePanel *panel = this->GetPanelFromPageId(loc_hint->PageId);
  if (!page || !panel)
    {
    return NULL;
    }

  vtkKWWidget *found = NULL;

  // Iterate over widget, find the one that has the same label and is packed
  // -in the same location (for extra safety reason, since several panels
  // or pages might have a widget with the same label)

  vtkCollectionIterator *child_it = 
    panel->GetPagesParentWidget()->GetChildren()->NewIterator();

  child_it->InitTraversal(); 
  while (!child_it->IsDoneWithTraversal())
    {
    vtkKWWidget *child = vtkKWWidget::SafeDownCast(child_it->GetCurrentObject());
    if (child && child->IsPacked())
      {
      const char *label = this->GetDragAndDropWidgetLabel(child);
      if (label && !strcmp(label, widget_label))
        {
        ostrstream in_str;
        if (vtkKWTkUtilities::GetPackSlaveIn(
              child->GetApplication()->GetMainInterp(), 
              child->GetWidgetName(), in_str))
          {
          in_str << ends;
          int cmp = strcmp(in_str.str(), page->GetWidgetName());
          in_str.rdbuf()->freeze(0);
          if (!cmp)
            {
            found = child;
            break;
            }
          }
        }
      }
    child_it->GoToNextItem();
    }
  child_it->Delete();

  return found;
}

//---------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::DragAndDropEntry::DragAndDropEntry()
{
  this->Widget = NULL;
}

//---------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::DragAndDropEntry* 
vtkKWUserInterfaceNotebookManager::GetLastDragAndDropEntry(vtkKWWidget *widget)
{
  vtkKWUserInterfaceNotebookManager::DragAndDropEntry *dd_entry = NULL;
  vtkKWUserInterfaceNotebookManager::DragAndDropEntry *found = NULL;
  vtkKWUserInterfaceNotebookManager::DragAndDropEntriesContainerIterator *it = 
    this->DragAndDropEntries->NewIterator();

  it->GoToLastItem();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(dd_entry) == VTK_OK && dd_entry->Widget == widget)
      {
      found = dd_entry;
      break;
      }
    it->GoToPreviousItem();
    }
  it->Delete();

  return found;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::GetNumberOfDragAndDropEntries()
{
  if (this->DragAndDropEntries)
    {
    return this->DragAndDropEntries->GetNumberOfItems();
    }
  return 0;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::DeleteAllDragAndDropEntries()
{
  if (!this->DragAndDropEntries)
    {
    return 0;
    }

  vtkKWUserInterfaceNotebookManager::DragAndDropEntry *dd_entry = NULL;
  vtkKWUserInterfaceNotebookManager::DragAndDropEntriesContainerIterator *it = 
    this->DragAndDropEntries->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(dd_entry) == VTK_OK)
      {
      delete dd_entry;
      }
    it->GoToNextItem();
    }
  it->Delete();

  this->DragAndDropEntries->RemoveAllItems();

  return 1;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::GetDragAndDropEntry(
  int idx, 
  ostream &widget_label, 
  ostream &from_panel_name, 
  ostream &from_page_title, 
  ostream &from_after_widget_label,
  ostream &to_panel_name, 
  ostream &to_page_title,
  ostream &to_after_widget_label)
{
  vtkKWUserInterfaceNotebookManager::DragAndDropEntry *dd_entry = NULL;
  if (this->LockDragAndDropEntries ||
      !this->DragAndDropEntries ||
      this->DragAndDropEntries->GetItem(idx, dd_entry) != VTK_OK ||
      !dd_entry)
    {
    return 0;
    }

  // Widget

  if (dd_entry->Widget)
    {
    widget_label << this->GetDragAndDropWidgetLabel(dd_entry->Widget);
    }

  // From
  // If both the page title and panel name are the same, do not
  // output the panel name

  vtkKWUserInterfaceNotebookManager::WidgetLocation *from_loc = 
    &dd_entry->FromLocation;

  const char *page_title = NULL;
  if (this->Notebook)
    {
    page_title = this->Notebook->GetPageTitle(from_loc->PageId);
    from_page_title << page_title;
    }
  vtkKWUserInterfacePanel *from_panel = 
    this->GetPanelFromPageId(from_loc->PageId);
  if (from_panel)
    {
    const char *panel_name = from_panel->GetName();
    if (panel_name && (!page_title || strcmp(panel_name, page_title)))
      {
      from_panel_name << panel_name;
      }
    }
  if (from_loc->AfterWidget)
    {
    from_after_widget_label << this->GetDragAndDropWidgetLabel(
      from_loc->AfterWidget);
    }

  // To
  // If From == To, do not output any location
  // If both the page title and panel name are the same, do not
  // output the page title

  vtkKWUserInterfaceNotebookManager::WidgetLocation *to_loc = 
    &dd_entry->ToLocation;

  if (from_loc->PageId != to_loc->PageId)
    {
    page_title = NULL;
    if (this->Notebook)
      {
      page_title = this->Notebook->GetPageTitle(to_loc->PageId);
      to_page_title << page_title;
      }
    vtkKWUserInterfacePanel *to_panel = 
      this->GetPanelFromPageId(to_loc->PageId);
    if (to_panel)
      {
      const char *panel_name = to_panel->GetName();
      if (panel_name && (!page_title || strcmp(panel_name, page_title)))
        {
        to_panel_name << panel_name;
        }
      }
    }
  if (to_loc->AfterWidget)
    {
    to_after_widget_label << this->GetDragAndDropWidgetLabel(
      to_loc->AfterWidget);
    }

  return 1;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::DragAndDropWidget(
  const char *widget_label, 
  const char *from_panel_name, 
  const char *from_page_title, 
  const char *from_after_widget_label,
  const char *to_panel_name, 
  const char *to_page_title,
  const char *to_after_widget_label)
{
  if (this->LockDragAndDropEntries || !this->Notebook || !widget_label)
    {
    return 0;
    }
  
  // From
  // If there is no panel name, use page title

  vtkKWUserInterfaceNotebookManager::WidgetLocation from_loc;

  if (!from_panel_name)
    {
    from_panel_name = from_page_title;
    }

  vtkKWUserInterfacePanel *from_panel = this->GetPanel(from_panel_name);
  if (from_panel && from_page_title)
    {
    if (!from_panel->IsCreated())
      {
      from_panel->Create(this->GetApplication());
      }
    int from_panel_id = this->GetPanelId(from_panel);
    if (this->Notebook->HasPage(from_page_title, from_panel_id))
      {
      from_loc.PageId = 
        this->Notebook->GetPageId(from_page_title, from_panel_id);
      }
    }
  if (from_after_widget_label)
    {
    from_loc.AfterWidget = this->GetDragAndDropWidgetFromLabelAndLocation(
      from_after_widget_label, &from_loc);
    }

  // Widget

  vtkKWWidget *widget = this->GetDragAndDropWidgetFromLabelAndLocation(
    widget_label, &from_loc);

  // To
  // If there if no "To page title", use "From page title"
  // If there is no panel name, use page title

  vtkKWUserInterfaceNotebookManager::WidgetLocation to_loc;

  if (!to_page_title)
    {
    to_page_title = from_page_title;
    }
  if (!to_panel_name)
    {
    to_panel_name = to_page_title;
    }

  vtkKWUserInterfacePanel *to_panel = this->GetPanel(to_panel_name);
  if (to_panel && to_page_title)
    {
    if (!to_panel->IsCreated())
      {
      to_panel->Create(this->GetApplication());
      }
    int to_panel_id = this->GetPanelId(to_panel);
    if (this->Notebook->HasPage(to_page_title, to_panel_id))
      {
      to_loc.PageId = 
        this->Notebook->GetPageId(to_page_title, to_panel_id);
      }
    }
  if (to_after_widget_label)
    {
    to_loc.AfterWidget = this->GetDragAndDropWidgetFromLabelAndLocation(
      to_after_widget_label, &to_loc);
    }

  // Move the widget

  this->DragAndDropWidget(widget, &from_loc, &to_loc);

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::DragAndDropWidget(
  vtkKWWidget *widget, 
  const WidgetLocation *from_loc,
  const WidgetLocation *to_loc)
{
  if (!widget || !from_loc || !to_loc ||
      !this->Notebook || !widget->IsCreated())
    {
    return 0;
    }

  // If a page id was specified, then pack in that specific page

  ostrstream in;
  vtkKWWidget *page = this->Notebook->GetFrame(to_loc->PageId);
  if (page)
    {
    in << " -in " << page->GetWidgetName();
    }
  in << ends;

  // If an "after widget" was specified, then pack after that widget

  ostrstream after;
  if (to_loc->AfterWidget && to_loc->AfterWidget->IsCreated())
    {
    after << " -after " << to_loc->AfterWidget->GetWidgetName();
    }
  after << ends;

  this->Notebook->Script(
    "pack %s -side top %s %s",
    widget->GetWidgetName(), in.str(), after.str()); 

  in.rdbuf()->freeze(0);
  after.rdbuf()->freeze(0);

  // Store the fact that this widget was moved

  this->AddDragAndDropEntry(widget, from_loc, to_loc);

  return 1;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::IsDragAndDropWidgetAtOriginalLocation(
  vtkKWWidget *widget)
{
  if (!widget)
    {
    return 0;
    }

  int atoriginal = 1;

  vtkKWUserInterfaceNotebookManager::DragAndDropEntry *dd_entry = NULL;
  vtkKWUserInterfaceNotebookManager::DragAndDropEntriesContainerIterator *it = 
    this->DragAndDropEntries->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(dd_entry) == VTK_OK && dd_entry->Widget == widget)
      {
      // Check that we have the same location, and that the after widget
      // is either NULL or a widget that has not moved either

      atoriginal = 
        (dd_entry->FromLocation.PageId == 
         dd_entry->ToLocation.PageId &&
         dd_entry->FromLocation.AfterWidget == 
         dd_entry->ToLocation.AfterWidget &&
         (dd_entry->ToLocation.AfterWidget == NULL ||
          this->IsDragAndDropWidgetAtOriginalLocation(
            dd_entry->ToLocation.AfterWidget)));
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return atoriginal;
}

//---------------------------------------------------------------------------
int vtkKWUserInterfaceNotebookManager::AddDragAndDropEntry(
  vtkKWWidget *widget, 
  const WidgetLocation *from_loc, 
  const WidgetLocation *to_loc)
{
  if (!widget || !from_loc || !to_loc)
    {
    return 0;
    }

  vtkKWUserInterfaceNotebookManager::DragAndDropEntry *dd_entry, *prev_entry;

  vtkKWUserInterfaceNotebookManager::WidgetLocation from_loc_fixed = *from_loc;

  // Do we have an entry for that widget already ?
  // In that case, remove it, and use the previous "from" location as the
  // current "from" location
  
  prev_entry = this->GetLastDragAndDropEntry(widget);
  if (prev_entry)
    {
    vtkIdType idx;
    if (!this->DragAndDropEntries->FindItem(prev_entry, idx) ||
        !this->DragAndDropEntries->RemoveItem(idx))
      {
      vtkErrorMacro(
        "Error while removing previous Drag & Drop entry from the manager.");
      return 0;
      }
    from_loc_fixed = prev_entry->FromLocation;
    }

  // Append and set an entry

  dd_entry = new vtkKWUserInterfaceNotebookManager::DragAndDropEntry;
  if (this->DragAndDropEntries->AppendItem(dd_entry) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a Drag & Drop entry to the manager.");
    delete dd_entry;
    return 0;
    }

  dd_entry->Widget = widget;
  dd_entry->FromLocation = from_loc_fixed;
  dd_entry->ToLocation = *to_loc;

  if (prev_entry)
    {
    delete prev_entry;
    }

  // Browse each entry for any entry representing a widget (W) 
  // dropped after the widget (A) we have moved. Since the location
  // if A is not valid anymore (it has been repack elsewhere), update the
  // old entry W so that its destination location matches its current location

  vtkKWUserInterfaceNotebookManager::DragAndDropEntriesContainerIterator *it = 
    this->DragAndDropEntries->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(dd_entry) == VTK_OK &&
        dd_entry->ToLocation.AfterWidget == widget)
      {
      this->GetDragAndDropWidgetLocation(
        dd_entry->Widget, &dd_entry->ToLocation);
      }
    it->GoToNextItem();
    }

  // Browse each entry, check if it represents an actual motion, if not 
  // then remove it

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(dd_entry) == VTK_OK && 
        this->IsDragAndDropWidgetAtOriginalLocation(dd_entry->Widget))
      {
      it->GoToNextItem();
      vtkIdType idx;
      if (this->DragAndDropEntries->FindItem(dd_entry, idx) &&
          this->DragAndDropEntries->RemoveItem(idx))
        {
        delete dd_entry;
        }
      else
        {
        vtkErrorMacro(
          "Error while removing noop Drag & Drop entry from the manager.");
        }
      }
    else
      {
      it->GoToNextItem();
      }
    }

#if 0
  cout << "-------------------------------" << endl;
  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(dd_entry) == VTK_OK)
      {
      cout << this->GetDragAndDropWidgetLabel(dd_entry->Widget) << " :\n";
      cout << " - From (" 
           << this->Notebook->GetPageTitle(dd_entry->FromLocation.PageId)
           << ", ";
      char *ptr = 
        this->GetDragAndDropWidgetLabel(dd_entry->FromLocation.AfterWidget);
      cout << (ptr ? ptr : "-") << ") " << endl;
      cout << " - To   (" 
           << this->Notebook->GetPageTitle(dd_entry->ToLocation.PageId)
           << ", ";
      ptr = 
        this->GetDragAndDropWidgetLabel(dd_entry->ToLocation.AfterWidget);
      cout << (ptr ? ptr : "-") << ") " << endl;
      }
    it->GoToNextItem();
    }
#endif

  it->Delete();
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::DragAndDropEndCallback(
  int x, int y, 
  vtkKWWidget *widget, vtkKWWidget *vtkNotUsed(anchor), vtkKWWidget *target)
{
  // The target must be the current notebook

  if (!this->Notebook || this->Notebook != target)
    {
    return;
    }

  // Get the current location of the widget

  vtkKWUserInterfaceNotebookManager::WidgetLocation from_loc;
  if (!this->GetDragAndDropWidgetLocation(widget, &from_loc))
    {
    return;
    }

  // If the target is a "tab" in the notebook, move the widget to the page
  // corresponding to that "tab"

  int page_id = this->Notebook->GetPageIdContainingCoordinatesInTab(x, y);
  if (page_id >= 0)
    {
    if (page_id != this->Notebook->GetRaisedPageId())
      {
      vtkKWUserInterfaceNotebookManager::WidgetLocation to_loc;
      to_loc.PageId = page_id;
      this->DragAndDropWidget(widget, &from_loc, &to_loc);
      }
    return;
    }

  // If not, first try to find the panel this widget is located in,
  // then browse the children of the panel to find the drop zone among the
  // sibling of the dragged widget

  vtkKWUserInterfacePanel *panel = this->GetPanelFromPageId(from_loc.PageId);
  if (!panel)
    {
    return;
    }

  vtkCollectionIterator *sibbling_it = 
    panel->GetPagesParentWidget()->GetChildren()->NewIterator();

  sibbling_it->InitTraversal(); 
  while (!sibbling_it->IsDoneWithTraversal())
    {
    vtkKWWidget *sibbling = 
      vtkKWWidget::SafeDownCast(sibbling_it->GetCurrentObject());
    vtkKWWidget *anchor = 0;

    // If a compliant sibbling was found, move the dragged widget after it
    
    if (sibbling != widget &&
        this->CanWidgetBeDragAndDropped(sibbling, &anchor) &&
        sibbling->IsMapped() && 
        vtkKWTkUtilities::ContainsCoordinates(
          sibbling->GetApplication()->GetMainInterp(),
          sibbling->GetWidgetName(),
          x, y))
      {
      vtkKWUserInterfaceNotebookManager::WidgetLocation to_loc;
      to_loc.PageId = from_loc.PageId;
      to_loc.AfterWidget = sibbling;
      this->DragAndDropWidget(widget, &from_loc, &to_loc);
      break;
      }
    sibbling_it->GoToNextItem();
    }
  sibbling_it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::PrintSelf(
  ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Notebook: " << this->Notebook << endl;
  os << indent << "EnableDragAndDrop: " 
     << (this->EnableDragAndDrop ? "On" : "Off") << endl;
  os << indent << "LockDragAndDropEntries: " 
     << (this->LockDragAndDropEntries ? "On" : "Off") << endl;
}
