/*=========================================================================

  Module:    vtkKWUserInterfaceManager.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWUserInterfaceManager.h"

#include "vtkKWWidget.h"
#include "vtkKWUserInterfacePanel.h"
#include "vtkObjectFactory.h"

#include <kwsys/stl/list>
#include <kwsys/stl/algorithm>

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWUserInterfaceManager, "1.17");

int vtkKWUserInterfaceManagerCommand(ClientData cd, Tcl_Interp *interp,
                                     int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWUserInterfaceManagerInternals
{
public:

  typedef kwsys_stl::list<vtkKWUserInterfaceManager::PanelSlot*> PanelsContainer;
  typedef kwsys_stl::list<vtkKWUserInterfaceManager::PanelSlot*>::iterator PanelsContainerIterator;

  PanelsContainer Panels;
};

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::vtkKWUserInterfaceManager()
{
  this->IdCounter = 0;

  this->Internals = new vtkKWUserInterfaceManagerInternals;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::~vtkKWUserInterfaceManager()
{
  // Delete all panels

  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        delete (*it);
        }
      }
    delete this->Internals;
    }
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetPanelSlot(vtkKWUserInterfacePanel *panel)
{
  if (this->Internals && panel)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel == panel)
        {
        return *it;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetPanelSlot(int id)
{
  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Id == id)
        {
        return *it;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetPanelSlot(const char *panel_name)
{
  if (this->Internals && panel_name)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel && (*it)->Panel->GetName() &&
          !strcmp((*it)->Panel->GetName(), panel_name))
        {
        return *it;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
vtkKWUserInterfaceManager::PanelSlot* 
vtkKWUserInterfaceManager::GetNthPanelSlot(int rank)
{
  if (this->Internals && rank >= 0 && rank < this->GetNumberOfPanels())
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && !rank--)
        {
        return *it;
        }
      }
    }

  return NULL;
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
vtkKWUserInterfacePanel* vtkKWUserInterfaceManager::GetNthPanel(int rank)
{
  vtkKWUserInterfaceManager::PanelSlot *panel_slot = 
    this->GetNthPanelSlot(rank);
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

// ---------------------------------------------------------------------------
int vtkKWUserInterfaceManager::IsCreated()
{
  return (this->GetApplication() != NULL);
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::SetEnabled(int arg)
{
  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel)
        {
        (*it)->Panel->SetEnabled(arg);
        (*it)->Panel->Update();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::Update()
{
  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel)
        {
        (*it)->Panel->Update();
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::UpdateEnableState()
{
  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel)
        {
        (*it)->Panel->UpdateEnableState();
        }
      }
    }
}

// ----------------------------------------------------------------------------
int vtkKWUserInterfaceManager::GetNumberOfPanels()
{
  return this->Internals ? this->Internals->Panels.size() : 0;
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

  this->Internals->Panels.push_back(panel_slot);
  
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

  vtkKWUserInterfaceManagerInternals::PanelsContainerIterator pos = 
    kwsys_stl::find(this->Internals->Panels.begin(),
                 this->Internals->Panels.end(),
                 panel_slot);

  if (pos == this->Internals->Panels.end())
    {
    vtkErrorMacro("Error while removing a panel from the manager "
                  "(can not find the panel).");
    return 0;
    }

  // Remove the panel from the container

  this->Internals->Panels.erase(pos);

  // Delete the panel slot

  delete panel_slot;

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::ShowAllPanels()
{
  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel)
        {
        this->ShowPanel((*it)->Panel);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::HideAllPanels()
{
  if (this->Internals)
    {
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator it = 
      this->Internals->Panels.begin();
    vtkKWUserInterfaceManagerInternals::PanelsContainerIterator end = 
      this->Internals->Panels.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Panel)
        {
        this->HidePanel((*it)->Panel);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWUserInterfaceManager::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
