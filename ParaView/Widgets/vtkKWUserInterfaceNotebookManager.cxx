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

#include "vtkKWUserInterfaceNotebookManager.h"

#include "vtkCollectionIterator.h"
#include "vtkKWApplication.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWNotebook.h"
#include "vtkKWSerializer.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWidgetCollection.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWUserInterfaceNotebookManager);
vtkCxxRevisionMacro(vtkKWUserInterfaceNotebookManager, "1.16");

int vtkKWUserInterfaceNotebookManagerCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::vtkKWUserInterfaceNotebookManager()
{
  // The parent class (vtkKWUserInterfaceManager) initializes IdCounter so that
  // panel IDs starts at 0. In this class, a panel ID will map to a notebook tag
  // (a tag is an integer associated to each notebook page). The default tag, 
  // if not specified, is 0. By starting at 1 we will avoid mixing managed 
  // and unmanaged pages (unmanaged pages are directly added to the notebook 
  // without going through the manager).

  this->IdCounter = 1;
  this->Notebook = NULL;
  this->EnableDragAndDrop = 0;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceNotebookManager::~vtkKWUserInterfaceNotebookManager()
{
  this->SetNotebook(NULL);
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
  // pages that share the same tag (i.e. among the pages that belong to the same 
  // panel). This allow pages from different panels to have the same title.

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
  // pages that share the same tag (i.e. among the pages that belong to the same 
  // panel). This allow pages from different panels to have the same title.

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
    panel->Create(this->Application);
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

  this->Notebook->HidePagesMatchingTag(tag);

  return 1;
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

// ---------------------------------------------------------------------------
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
    vtkErrorMacro("Can not update a panel that is not "
                  "in the manager.");
    return;
    }

  if (!this->Notebook)
    {
    return;
    }

  // Get the pages parent, and check if there are widgets (here, labeled frame)
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
    // Check if the widget is a labeled frame. If we did not find such a frame,
    // check if it is a widget with a single labeled frame child (i.e. a 
    // vtkKWWidget that uses a labeled frame but did not want to inherit from 
    // vtkKWLabeledFrame)

    vtkKWWidget *widget = 0;
    vtkKWLabeledFrame *frame = vtkKWLabeledFrame::SafeDownCast(it->GetObject());
    if (frame)
      {
      widget = frame;
      }
    else
      {
      widget = vtkKWWidget::SafeDownCast(it->GetObject());
      if (widget && widget->GetChildren()->GetNumberOfItems() == 1)
        {
        frame = vtkKWLabeledFrame::SafeDownCast(
          widget->GetChildren()->GetLastKWWidget());
        }
      }

    // Enable/Disable Drag & Drop for that frame, the notebook is the drop target

    if (widget && frame)
      {
      if (this->EnableDragAndDrop)
        {
        if (!widget->HasDragAndDropTarget(this->Notebook))
          {
          widget->EnableDragAndDropOn();
          widget->SetDragAndDropAnchor(frame->GetDragAndDropAnchor());
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

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::DragAndDropEndCallback(
  int x, int y, 
  vtkKWWidget *widget, vtkKWWidget *vtkNotUsed(anchor), vtkKWWidget *target)
{
  if (!this->Notebook || this->Notebook != target)
    {
    return;
    }

  // If the target is a tab in the notebook, move the widget to the page

  int page_id = this->Notebook->GetPageIdContainingCoordinatesInTab(x, y);
  if (page_id >= 0)
    {
    if (page_id != this->Notebook->GetRaisedPageId())
      {
      this->Notebook->Script("pack %s -side top -in %s",
                             widget->GetWidgetName(), 
                             this->Notebook->GetFrame(page_id)->GetWidgetName());
      }
    return;
    }

  // If not, first try to find the panel which is the parent of the dragged 
  // widget

  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelSlot *panel_found = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *panel_it = 
    this->Panels->NewIterator();

  panel_it->InitTraversal();
  while (!panel_it->IsDoneWithTraversal())
    {
    if (panel_it->GetData(panel_slot) == VTK_OK && 
        panel_slot->Panel->GetPagesParentWidget() == widget->GetParent())
      {
      panel_found = panel_slot;
      break;
      }
    panel_it->GoToNextItem();
    }
  panel_it->Delete();

  if (!panel_found)
    {
    return;
    }
 
  // Then browse the children of the panel to find the drop zone among the
  // sibling of the dragged widget

  vtkCollectionIterator *child_it = 
    panel_found->Panel->GetPagesParentWidget()->GetChildren()->NewIterator();

  child_it->InitTraversal(); 
  while (!child_it->IsDoneWithTraversal())
    {
    // Check if the child is a labeled frame. If we did not find such a frame,
    // check if it is a widget with a single labeled frame child (i.e. a 
    // vtkKWWidget that uses a labeled frame but did not want to inherit from 
    // vtkKWLabeledFrame)

    vtkKWWidget *child = 0;
    vtkKWLabeledFrame *child_frame = 
      vtkKWLabeledFrame::SafeDownCast(child_it->GetObject());
    if (child_frame)
      {
      child = child_frame;
      }
    else
      {
      child = vtkKWWidget::SafeDownCast(child_it->GetObject());
      if (child && child->GetChildren()->GetNumberOfItems() == 1)
        {
        child_frame = vtkKWLabeledFrame::SafeDownCast(
          child->GetChildren()->GetLastKWWidget());
        }
      }

    // If the correct child was found, pack the dragged widget after it

    if (child && child_frame && child != widget && child->IsMapped() &&
        vtkKWTkUtilities::ContainsCoordinates(
          child->GetApplication()->GetMainInterp(),
          child->GetWidgetName(),
          x, y))
      {
      this->Notebook->Script("pack %s -after %s",
                             widget->GetWidgetName(), child->GetWidgetName());
      break;
      }

    child_it->GoToNextItem();
    }
  child_it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Notebook: " << this->Notebook << endl;
  os << indent << "EnableDragAndDrop: " 
     << (this->EnableDragAndDrop ? "On" : "Off") << endl;
}


