/*=========================================================================

  Module:    vtkKWToolbarSet.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkKWToolbarSet.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"

#if defined(_WIN32)
#define VTK_KW_TOOLBAR_RELIEF_SEP "groove"
#else
#define VTK_KW_TOOLBAR_RELIEF_SEP "sunken"
#endif

#include <kwsys/stl/list>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWToolbarSet);
vtkCxxRevisionMacro(vtkKWToolbarSet, "1.10");

int vtkvtkKWToolbarSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWToolbarSetInternals
{
public:

  typedef kwsys_stl::list<vtkKWToolbarSet::ToolbarSlot*> ToolbarsContainer;
  typedef kwsys_stl::list<vtkKWToolbarSet::ToolbarSlot*>::iterator ToolbarsContainerIterator;

  ToolbarsContainer Toolbars;
};

//----------------------------------------------------------------------------
vtkKWToolbarSet::vtkKWToolbarSet()
{
  this->ShowBottomSeparator  = 1;

  this->ToolbarsFrame        = vtkKWFrame::New();
  this->BottomSeparatorFrame = vtkKWFrame::New();

  this->Internals = new vtkKWToolbarSetInternals;
}

//----------------------------------------------------------------------------
vtkKWToolbarSet::~vtkKWToolbarSet()
{
  this->ToolbarsFrame->Delete();
  this->BottomSeparatorFrame->Delete();

  // Delete all toolbars

  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        if ((*it)->SeparatorFrame)
          {
          (*it)->SeparatorFrame->Delete();
          }
        delete (*it);
        }
      }
    delete this->Internals;
    }
}

//----------------------------------------------------------------------------
vtkKWToolbarSet::ToolbarSlot* 
vtkKWToolbarSet::GetToolbarSlot(vtkKWToolbar *toolbar)
{
  if (this->Internals && toolbar)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Toolbar == toolbar)
        {
        return *it;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::Create(vtkKWApplication *app, const char *args)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->Superclass::Create(app, "frame", args))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  // Create the toolbars frame container

  this->ToolbarsFrame->SetParent(this);  
  this->ToolbarsFrame->Create(app, "");

  // Bottom separator

  this->BottomSeparatorFrame->SetParent(this);  
  this->BottomSeparatorFrame->Create(app, "-height 2 -bd 1");

  this->Script("%s config -relief %s", 
               this->BottomSeparatorFrame->GetWidgetName(), 
               VTK_KW_TOOLBAR_RELIEF_SEP);

  this->Update();
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::Update()
{
  // Pack

  this->Pack();
  
  // Update enable state

  this->UpdateEnableState();
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::Pack()
{
  if (this->IsCreated())
    {
    this->Script("pack %s -side top -fill both -expand y -padx 0 -pady 0",
                 this->ToolbarsFrame->GetWidgetName());
    }

  this->PackToolbars();
  this->PackBottomSeparator();
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::PackBottomSeparator()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->ShowBottomSeparator)
    {
    this->Script(
      "pack %s -side top -fill x -expand y -padx 0 -pady 2 -after %s",
      this->BottomSeparatorFrame->GetWidgetName(),
      this->ToolbarsFrame->GetWidgetName());
    }
  else
    {
    this->BottomSeparatorFrame->Unpack();
    }
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::PackToolbars()
{
  if (!this->IsCreated() || !this->Internals)
    {
    return;
    }

  this->ToolbarsFrame->UnpackChildren();

  if (!this->GetNumberOfVisibleToolbars())
    {
    return;
    }

  ostrstream tk_cmd;

  vtkKWToolbar *previous = NULL;

  vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
    this->Internals->Toolbars.begin();
  vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
    this->Internals->Toolbars.end();
  for (; it != end; ++it)
    {
    if (*it && (*it)->Toolbar && (*it)->Toolbar->IsCreated())
      {
      if ((*it)->Visibility)
        {
        // Pack a separator
        (*it)->Toolbar->Bind();
        if (previous)
          {
          if (!(*it)->SeparatorFrame->IsCreated())
            {
            (*it)->SeparatorFrame->SetParent(this->ToolbarsFrame);
            (*it)->SeparatorFrame->Create(
              this->GetApplication(), "-width 2 -bd 1");
            this->Script("%s config -relief %s", 
                         (*it)->SeparatorFrame->GetWidgetName(), 
                         VTK_KW_TOOLBAR_RELIEF_SEP);
            }
          tk_cmd << "pack " << (*it)->SeparatorFrame->GetWidgetName() 
                 << " -side left -padx 1 -pady 0 -fill y -expand n" << endl;
          }
        previous = (*it)->Toolbar;

        // Pack toolbar

        tk_cmd << "pack " << (*it)->Toolbar->GetWidgetName() 
               << " -side left -padx 1 -pady 0 -fill both -expand "
               << ((*it)->Toolbar->GetResizable() ? "y" : "n")
               << " -in " << this->ToolbarsFrame->GetWidgetName() << endl;
        }
      else
        {
        // Unpack separator and toolbar
        (*it)->Toolbar->UnBind();
        if ((*it)->SeparatorFrame->IsCreated())
          {
          tk_cmd << "pack forget " 
                 << (*it)->SeparatorFrame->GetWidgetName() << endl;
          }
        tk_cmd << "pack forget " 
               << (*it)->Toolbar->GetWidgetName() << endl;
        }
      }
    }

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::HasToolbar(vtkKWToolbar *toolbar)
{
  return this->GetToolbarSlot(toolbar) ? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::AddToolbar(vtkKWToolbar *toolbar)
{
  // Check if the new toolbar is already in

  if (this->HasToolbar(toolbar))
    {
    vtkErrorMacro("The toolbar already exists in the toolbar set.");
    return 0;
    }

  // Add the toolbar slot to the manager

  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = 
    new vtkKWToolbarSet::ToolbarSlot;

  this->Internals->Toolbars.push_back(toolbar_slot);

  // Create the toolbar

  toolbar_slot->Visibility = 1;

  toolbar_slot->Toolbar = toolbar;
  toolbar_slot->Toolbar->SetEnabled(this->Enabled);

  toolbar_slot->SeparatorFrame = vtkKWFrame::New();
  toolbar_slot->SeparatorFrame->SetEnabled(this->Enabled);

  // Pack the toolbars

  this->PackToolbars();

  return 1;
}

// ----------------------------------------------------------------------------
vtkIdType vtkKWToolbarSet::GetNumberOfToolbars()
{
  return this->Internals->Toolbars.size();
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::HideToolbar(vtkKWToolbar *toolbar)
{
  this->SetToolbarVisibility(toolbar, 0);
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::ShowToolbar(vtkKWToolbar *toolbar)
{
  this->SetToolbarVisibility(toolbar, 1);
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarVisibility(
  vtkKWToolbar *toolbar, int flag)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);

  if (toolbar_slot && toolbar_slot->Visibility != flag)
    {
    toolbar_slot->Visibility = flag;
    this->PackToolbars();
    }
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::IsToolbarVisible(vtkKWToolbar* toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);
  return (toolbar_slot && toolbar_slot->Visibility)? 1 : 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkKWToolbarSet::GetNumberOfVisibleToolbars()
{
  int count = 0;

  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Visibility)
        {
        ++count;
        }
      }
    }

  return count;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarsFlatAspect(int f)
{
  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Toolbar)
        {
        (*it)->Toolbar->SetFlatAspect(f);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarsWidgetsFlatAspect(int f)
{
  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Toolbar)
        {
        (*it)->Toolbar->SetWidgetsFlatAspect(f);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetShowBottomSeparator(int arg)
{
  if (this->ShowBottomSeparator == arg)
    {
    return;
    }

  this->ShowBottomSeparator = arg;
  this->Modified();

  this->PackBottomSeparator();
}

//----------------------------------------------------------------------------
vtkKWToolbar* vtkKWToolbarSet::GetToolbar(int index)
{
  if (index >= 0 && index < this->GetNumberOfToolbars() && this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && !index--)
        {
        return (*it)->Toolbar;
        }
      }
    }

  return NULL;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it)
        {
        if ((*it)->Toolbar)
          {
          (*it)->Toolbar->SetEnabled(this->Enabled);
          }
        if ((*it)->SeparatorFrame)
          {
          (*it)->SeparatorFrame->SetEnabled(this->Enabled);
          }
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ToolbarsFrame: " << this->ToolbarsFrame << endl;
  os << indent << "ShowBottomSeparator: " 
     << (this->ShowBottomSeparator ? "On" : "Off") << endl;
}

