/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWUserInterfaceNotebookManager.cxx
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

#include "vtkKWUserInterfaceNotebookManager.h"

#include "vtkCollectionIterator.h"
#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWNotebook.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkKWWidgetCollection.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWUserInterfaceNotebookManager);
vtkCxxRevisionMacro(vtkKWUserInterfaceNotebookManager, "1.5");

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
  vtkKWUserInterfacePanel *panel)
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

  // Get the pages parent, and check if there are labeled frame that can be
  // switch from one page to the other (share the same parent)

  vtkKWWidget *parent = this->GetPagesParentWidget(panel);
  if (!parent)
    {
    return;
    }

  vtkCollectionIterator *it = parent->GetChildren()->NewIterator();
  for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
    vtkKWLabeledFrame *frame = vtkKWLabeledFrame::SafeDownCast(it->GetObject());
    if (!frame)
      {
      vtkKWWidget *widget = vtkKWWidget::SafeDownCast(it->GetObject());
      if (widget && widget->GetChildren()->GetNumberOfItems() == 1)
        {
        frame = vtkKWLabeledFrame::SafeDownCast(
          widget->GetChildren()->GetLastKWWidget());
        }
      }
    if (frame)
      {
      // cout << panel->GetName() << " : " << frame->GetLabel()->GetLabel() << endl;
      }
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceNotebookManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Notebook: " << this->Notebook << endl;
}

