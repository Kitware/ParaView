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

#include "vtkKWUserInterfaceManager.h"

#include "vtkKWWidget.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWUserInterfaceManager, "1.11");

int vtkKWUserInterfaceManagerCommand(ClientData cd, Tcl_Interp *interp,
                                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::vtkKWUserInterfaceManager()
{
  this->IdCounter = 0;
  this->Panels = vtkKWUserInterfaceManager::PanelsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::~vtkKWUserInterfaceManager()
{
  // Delete all panels

  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK)
      {
      delete panel_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->Panels->Delete();
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetPanelSlot(vtkKWUserInterfacePanel *panel)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelSlot *found = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK && panel_slot->Panel == panel)
      {
      found = panel_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetPanelSlot(int id)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelSlot *found = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK && panel_slot->Id == id)
      {
      found = panel_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetPanelSlot(const char *panel_name)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelSlot *found = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK && 
        panel_slot->Panel &&
        panel_slot->Panel->GetName() &&
        !strcmp(panel_slot->Panel->GetName(), panel_name))
      {
      found = panel_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManager::HasPanel(vtkKWUserInterfacePanel *panel)
{
  return this->GetPanelSlot(panel) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManager::GetPanelId(vtkKWUserInterfacePanel *panel)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = this->GetPanelSlot(panel);
  if (!panel_slot)
    {
    return -1;
    }

  return panel_slot->Id;
}

//----------------------------------------------------------------------------
vtkKWUserInterfacePanel* vtkKWUserInterfaceManager::GetPanel(int id)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = this->GetPanelSlot(id);
  if (!panel_slot)
    {
    return NULL;
    }

  return panel_slot->Panel;
}

//----------------------------------------------------------------------------
vtkKWUserInterfacePanel* vtkKWUserInterfaceManager::GetPanel(
  const char *panel_name)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = 
    this->GetPanelSlot(panel_name);
  if (!panel_slot)
    {
    return NULL;
    }

  return panel_slot->Panel;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::Create(vtkKWApplication *app)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The manager is already created");
    return;
    }

  this->SetApplication(app);
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::SetEnabled(int arg)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK)
      {
      panel_slot->Panel->SetEnabled(arg);
      panel_slot->Panel->Update();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::Update()
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK)
      {
      panel_slot->Panel->Update();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::UpdateEnableState()
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK)
      {
      panel_slot->Panel->UpdateEnableState();
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManager::AddPanel(vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    vtkErrorMacro("Can not add a NULL panel to the manager.");
    return -1;
    }
  
  // Don't allow duplicates (return silently though)

  if (this->HasPanel(panel))
    {
    return this->GetPanelId(panel);
    }

  // Add the panel slot to the manager

  vtkKWUserInterfaceManager::PanelSlot *panel_slot = 
    new vtkKWUserInterfaceManager::PanelSlot;

  if (this->Panels->AppendItem(panel_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a panel to the manager.");
    delete panel_slot;
    return -1;
    }
  
  // Each panel has a unique ID in the manager lifetime

  panel_slot->Panel = panel;
  panel_slot->Id = this->IdCounter++;

  return panel_slot->Id;
}

//----------------------------------------------------------------------------
int vtkKWUserInterfaceManager::RemovePanel(vtkKWUserInterfacePanel *panel)
{
  if (!panel)
    {
    vtkErrorMacro("Can not remove a NULL panel from the manager.");
    return 0;
    }
  
  // Silently returns if the panel has been removed already

  if (!this->HasPanel(panel))
    {
    return 1;
    }

  // Remove the page widgets from the interface

  this->RemovePageWidgets(panel);

  // Find the panel in the manager

  vtkKWUserInterfaceManager::PanelSlot *panel_slot = this->GetPanelSlot(panel);

  vtkIdType pos = 0;
  if (this->Panels->FindItem(panel_slot, pos) != VTK_OK)
    {
    vtkErrorMacro("Error while removing a panel from the manager "
                  "(can not find the panel).");
    return 0;
    }

  // Remove the panel from the container

  if (this->Panels->RemoveItem(pos) != VTK_OK)
    {
    vtkErrorMacro("Error while removing a panel from the manager.");
    return 0;
    }

  // Delete the panel slot

  delete panel_slot;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::ShowAllPanels()
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = NULL;
  vtkKWUserInterfaceManager::PanelsContainerIterator *it = 
    this->Panels->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(panel_slot) == VTK_OK && panel_slot->Panel)
      {
      this->ShowPanel(panel_slot->Panel);
      }
    it->GoToNextItem();
    }
  it->Delete();
}
