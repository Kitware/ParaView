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
#include "vtkKWInternationalization.h"
#include "vtkKWMenu.h"
#include "vtkKWSeparator.h"
#include "vtkKWTkUtilities.h"
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"

#include <vtksys/stl/list>
#include <vtksys/SystemTools.hxx>
#include <vtksys/stl/algorithm>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWToolbarSet);
vtkCxxRevisionMacro(vtkKWToolbarSet, "1.41");

//----------------------------------------------------------------------------
class vtkKWToolbarSetInternals
{
public:

  typedef vtksys_stl::list<vtkKWToolbarSet::ToolbarSlot*> ToolbarsContainer;
  typedef vtksys_stl::list<vtkKWToolbarSet::ToolbarSlot*>::iterator ToolbarsContainerIterator;

  ToolbarsContainer Toolbars;

  vtksys_stl::string PreviousPackInfo;
  vtksys_stl::string PreviousGridInfo;
};

//----------------------------------------------------------------------------
vtkKWToolbarSet::vtkKWToolbarSet()
{
  this->BottomSeparatorVisibility  = 0;
  this->TopSeparatorVisibility  = 0;
  this->SynchronizeToolbarsVisibilityWithRegistry  = 0;

  this->ToolbarVisibilityChangedCommand  = NULL;
  this->NumberOfToolbarsChangedCommand  = NULL;

  this->ToolbarsFrame   = vtkKWFrame::New();
  this->BottomSeparator = vtkKWSeparator::New();
  this->TopSeparator    = vtkKWSeparator::New();

  this->Internals = new vtkKWToolbarSetInternals;
}

//----------------------------------------------------------------------------
vtkKWToolbarSet::~vtkKWToolbarSet()
{
  if (this->ToolbarsFrame)
    {
    this->ToolbarsFrame->Delete();
    this->ToolbarsFrame = NULL;
    }

  if (this->BottomSeparator)
    {
    this->BottomSeparator->Delete();
    this->BottomSeparator = NULL;
    }

  if (this->TopSeparator)
    {
    this->TopSeparator->Delete();
    this->TopSeparator = NULL;
    }

  if (this->ToolbarVisibilityChangedCommand)
    {
    delete [] this->ToolbarVisibilityChangedCommand;
    this->ToolbarVisibilityChangedCommand = NULL;
    }

  if (this->NumberOfToolbarsChangedCommand)
    {
    delete [] this->NumberOfToolbarsChangedCommand;
    this->NumberOfToolbarsChangedCommand = NULL;
    }

  // Delete all toolbars

  this->RemoveAllToolbars();
  if (this->Internals)
    {
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
void vtkKWToolbarSet::CreateWidget()
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro(<< this->GetClassName() << " already created");
    return;
    }

  // Call the superclass to create the whole widget

  this->Superclass::CreateWidget();

  // Create the toolbars frame container

  this->ToolbarsFrame->SetParent(this);  
  this->ToolbarsFrame->Create();

  // Bottom separator

  this->BottomSeparator->SetParent(this);  
  this->BottomSeparator->Create();
  this->BottomSeparator->SetOrientationToHorizontal();

  // Top separator

  this->TopSeparator->SetParent(this);  
  this->TopSeparator->Create();
  this->TopSeparator->SetOrientationToHorizontal();

  // This is needed for the hack in Pack() to work, otherwise
  // the widget is not hidden properly if the user packs it manually
  // and don't update it after that (i.e. call a method that would
  // call Pack()). This will likely force Pack() to be called
  // twice each time we add a toolbar, but it's not like we have
  // thousand's of them. Again, hack, sorry. 
  
  this->AddBinding("<Map>", this, "Pack");

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
  this->PackToolbars();
  this->PackBottomSeparator();
  this->PackTopSeparator();

  // This is a hack. I tried hard to solve the problem, without success.
  // It seems that no matter what, if you pack the toolbar set, show
  // some toolbars, them hide all of them, the toolbarset frame does not
  // collapse automatically, but keeps its previous height, even
  // if all the widgets packed around are setup to reclaim the space that 
  // would have been left by an empty toolbar set.
  // Furthermore, it seems you can not even pack or grid an empty frame
  // it showing up as a one pixel high frame.
  // This is annoying. One solution would to add a method to allow the
  // user to set a callback to invoke each time the number of visible toolbars
  // change. This callback could in turn pack/unpack the whole toolbarset
  // if no toolbars are available. This would put the burden on the users, and
  // I would rather have that class solves its own problems.
  // Here is one hack. If nothing is visible, and the height is higher
  // than 1, and it is likely we did not collapse correctly, then we
  // should manually set the height to be 1 (sadly, it can not be 0:
  // "if this option is less "than or equal to zero then the window 
  // will not request any size at all"). Well that extra pixel still shows.
  // Here is another hack. Hide the whole widget by saving the previous
  // packing or gridding info, unpack it, and restore it later on. Evil.

  // If we are mapped, and we do not have any toolbar, 
  // save the widget's layout info and remove the widget from the layout

  if (this->IsCreated())
    {
    if (!this->GetNumberOfVisibleToolbars())
      {
      // Store the previous info

      if (this->IsPacked())
        {
        this->Internals->PreviousPackInfo = 
          this->Script("pack info %s", this->GetWidgetName());
        
        // Tk bug/feature ? The -after or -before parameter are not returned
        // by pack. Let's find them manually.

        if (!this->Internals->PreviousPackInfo.empty())
          {
          ostrstream master, previous_slave, next_slave;
          
          vtkKWTkUtilities::GetMasterInPack(this, master);
          master << ends;

          vtkKWTkUtilities::GetPreviousAndNextSlaveInPack(
            this->GetApplication()->GetMainInterp(),
            master.str(), this->GetWidgetName(), previous_slave, next_slave);
          previous_slave << ends;
          next_slave << ends;
          if (*previous_slave.str())
            {
            this->Internals->PreviousPackInfo += " -after ";
            this->Internals->PreviousPackInfo += previous_slave.str();
            }
          else if (*next_slave.str())
            {
            this->Internals->PreviousPackInfo += " -before ";
            this->Internals->PreviousPackInfo += next_slave.str();
            }
          master.rdbuf()->freeze(0);
          previous_slave.rdbuf()->freeze(0);
          next_slave.rdbuf()->freeze(0);
          }
        this->Script("pack forget %s", this->GetWidgetName());
        this->Internals->PreviousGridInfo.assign("");
        }
      else
        {
        vtksys_stl::string grid_info = 
          this->Script("grid info %s", this->GetWidgetName());
        if (!grid_info.empty())
          {
          this->Internals->PreviousPackInfo.assign("");
          this->Internals->PreviousGridInfo = grid_info;
          this->Script("grid forget %s", this->GetWidgetName());
          }
        }
      }

    // If we are not mapped, and we do have toolbars, 
    // restore the widget's layout info

    else
      {
      // Restore the previous info

      if (!this->Internals->PreviousPackInfo.empty())
        {
        this->Script("pack %s %s", this->GetWidgetName(), 
                     this->Internals->PreviousPackInfo.c_str());
        this->Internals->PreviousPackInfo.assign("");
        this->Internals->PreviousGridInfo.assign("");
        }
      else if (!this->Internals->PreviousGridInfo.empty())
        {
        this->Script("grid %s %s", this->GetWidgetName(), 
                     this->Internals->PreviousGridInfo.c_str());
        this->Internals->PreviousPackInfo.assign("");
        this->Internals->PreviousGridInfo.assign("");
        }
      }
    }
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::PackBottomSeparator()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->BottomSeparator)
    {
    if (this->BottomSeparatorVisibility && this->GetNumberOfVisibleToolbars())
      {
      this->Script(
        "pack %s -side top -fill x -expand y -padx 0 -pady 2 -after %s",
        this->BottomSeparator->GetWidgetName(),
        this->ToolbarsFrame->GetWidgetName());
      }
    else
      {
      this->BottomSeparator->Unpack();
      }
    }
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::PackTopSeparator()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->TopSeparator)
    {
    if (this->TopSeparatorVisibility && this->GetNumberOfVisibleToolbars())
      {
      this->Script(
        "pack %s -side top -fill x -expand y -padx 0 -pady 2 -before %s",
        this->TopSeparator->GetWidgetName(),
        this->ToolbarsFrame->GetWidgetName());
      }
    else
      {
      this->TopSeparator->Unpack();
      }
    }
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::PackToolbars()
{
  if (!this->IsCreated() || !this->Internals || !this->ToolbarsFrame)
    {
    return;
    }

  this->ToolbarsFrame->UnpackChildren();

  if (!this->GetNumberOfVisibleToolbars())
    {
    this->ToolbarsFrame->Unpack();
    return;
    }

  ostrstream tk_cmd;

  tk_cmd << "pack " << this->ToolbarsFrame->GetWidgetName() 
         << " -side top -fill both -expand y -padx 0 -pady 0" << endl;

  vtkKWToolbar *previous_w = NULL, *previous_e = NULL;

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
        int anchor_w = ((*it)->Anchor == vtkKWToolbarSet::ToolbarAnchorWest);
        const char *side = anchor_w ? " -side left" : " -side right";

        // Pack a separator

        //(*it)->Toolbar->Bind();
        if ((anchor_w && previous_w) || (!anchor_w && previous_e))
          {
          if (!(*it)->Separator->IsCreated())
            {
            (*it)->Separator->SetParent(this->ToolbarsFrame);
            (*it)->Separator->Create();
            (*it)->Separator->SetOrientationToVertical();
            }
          tk_cmd << "pack " << (*it)->Separator->GetWidgetName() 
                 << " -padx 1 -pady 0 -fill y -expand n " << side << endl;
          }

        // Pack toolbar

        tk_cmd << "pack " << (*it)->Toolbar->GetWidgetName() 
               << " -padx 1 -pady 0 -anchor w " << side
               << " -in " << this->ToolbarsFrame->GetWidgetName()
               << " -fill both -expand "
               << ((*it)->Toolbar->GetResizable() ? "y" : "n")
               << endl;
        
        if (anchor_w)
          {
          previous_w = (*it)->Toolbar;
          }
        else
          {
          previous_e = (*it)->Toolbar;
          }
        }
      else
        {
        // Unpack separator and toolbar
        //(*it)->Toolbar->UnBind();
        if ((*it)->Separator->IsCreated())
          {
          tk_cmd << "pack forget " 
                 << (*it)->Separator->GetWidgetName() << endl;
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
int vtkKWToolbarSet::AddToolbar(vtkKWToolbar *toolbar, int default_visibility)
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

  toolbar_slot->Toolbar = toolbar;
  this->PropagateEnableState(toolbar_slot->Toolbar);

  toolbar_slot->Separator = vtkKWSeparator::New();
  this->PropagateEnableState(toolbar_slot->Separator);

  toolbar_slot->Anchor = vtkKWToolbarSet::ToolbarAnchorWest;
  toolbar_slot->Visibility = default_visibility;
  if (this->SynchronizeToolbarsVisibilityWithRegistry)
    {
    this->RestoreToolbarVisibilityFromRegistry(toolbar_slot->Toolbar);
    }

  toolbar_slot->Toolbar->Register(this);

  // Pack the toolbars if we just brought a visible toolbar

  if (toolbar_slot->Visibility)
    {
    this->Pack();
    }

  this->InvokeNumberOfToolbarsChangedCommand();

  return 1;
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::RemoveToolbar(vtkKWToolbar *toolbar)
{
  // Check if the toolbar is in

  if (!this->HasToolbar(toolbar))
    {
    vtkErrorMacro("The toolbar is not in the toolbar set.");
    return 0;
    }

  // Find the panel in the manager

  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);

  vtkKWToolbarSetInternals::ToolbarsContainerIterator pos = 
    vtksys_stl::find(this->Internals->Toolbars.begin(),
                 this->Internals->Toolbars.end(),
                 toolbar_slot);

  if (pos == this->Internals->Toolbars.end())
    {
    vtkErrorMacro("Error while removing a toolbar from the set "
                  "(can not find the toolbar).");
    return 0;
    }

  // Remove the toolbar from the container
  
  this->Internals->Toolbars.erase(pos);

  // Pack the toolbars if we just removed a visible toolbar

  if (toolbar_slot->Visibility)
    {
    this->Pack();
    }

  // Delete the toolbar slot

  if (toolbar_slot->Separator)
    {
    toolbar_slot->Separator->Delete();
    }
  
  toolbar_slot->Toolbar->UnRegister(this);

  delete toolbar_slot;

  this->InvokeNumberOfToolbarsChangedCommand();

  return 1;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::RemoveAllToolbars()
{
  while (this->GetNumberOfToolbars())
    {
    this->RemoveToolbar(this->GetNthToolbar(0));
    }
}

// ----------------------------------------------------------------------------
int vtkKWToolbarSet::GetNumberOfToolbars()
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
void vtkKWToolbarSet::ToggleToolbarVisibility(vtkKWToolbar *toolbar)
{
  this->SetToolbarVisibility(
    toolbar, this->GetToolbarVisibility(toolbar) ? 0 : 1);
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarVisibility(
  vtkKWToolbar *toolbar, int flag)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);

  if (toolbar_slot && toolbar_slot->Visibility != flag)
    {
    toolbar_slot->Visibility = flag;
    if (this->SynchronizeToolbarsVisibilityWithRegistry)
      {
      this->SaveToolbarVisibilityToRegistry(toolbar_slot->Toolbar);
      }
    this->Pack();
    this->InvokeToolbarVisibilityChangedCommand(toolbar);
    }
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::GetToolbarVisibility(vtkKWToolbar* toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);
  return (toolbar_slot && toolbar_slot->Visibility)? 1 : 0;
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::GetNumberOfVisibleToolbars()
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
void vtkKWToolbarSet::SetToolbarAnchor(
  vtkKWToolbar *toolbar, int anchor)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);

  if (toolbar_slot && toolbar_slot->Anchor != anchor)
    {
    toolbar_slot->Anchor = anchor;
    if (toolbar_slot->Visibility)
      {
      this->Pack();
      }
    }
}

//----------------------------------------------------------------------------
int vtkKWToolbarSet::GetToolbarAnchor(vtkKWToolbar* toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);
  if (toolbar_slot)
    {
    return toolbar_slot->Anchor;
    }
  return vtkKWToolbarSet::ToolbarAnchorWest;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarsAspect(int f)
{
  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Toolbar && 
          (*it)->Toolbar->GetToolbarAspect() != 
          vtkKWToolbar::ToolbarAspectUnChanged)
        {
        (*it)->Toolbar->SetToolbarAspect(f);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarsWidgetsAspect(int f)
{
  if (this->Internals)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Toolbar &&
          (*it)->Toolbar->GetWidgetsAspect() != 
          vtkKWToolbar::WidgetsAspectUnChanged)
        {
        (*it)->Toolbar->SetWidgetsAspect(f);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetBottomSeparatorVisibility(int arg)
{
  if (this->BottomSeparatorVisibility == arg)
    {
    return;
    }

  this->BottomSeparatorVisibility = arg;
  this->Modified();

  this->PackBottomSeparator();
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetTopSeparatorVisibility(int arg)
{
  if (this->TopSeparatorVisibility == arg)
    {
    return;
    }

  this->TopSeparatorVisibility = arg;
  this->Modified();

  this->PackTopSeparator();
}

//----------------------------------------------------------------------------
vtkKWToolbar* vtkKWToolbarSet::GetNthToolbar(int index)
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
void vtkKWToolbarSet::SaveToolbarVisibilityToRegistry(
  vtkKWToolbar *toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);
  if (toolbar_slot && 
      toolbar_slot->Toolbar && 
      toolbar_slot->Toolbar->GetName())
    {
    char *clean_name = vtksys::SystemTools::RemoveChars(
      toolbar_slot->Toolbar->GetName(), " ");
    vtksys_stl::string key(clean_name);
    delete [] clean_name;

    key += "Visibility";
    this->GetApplication()->SetRegistryValue(
      2, "Toolbars", key.c_str(), "%d", toolbar_slot->Visibility);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::RestoreToolbarVisibilityFromRegistry(
  vtkKWToolbar *toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);
  if (toolbar_slot && 
      toolbar_slot->Toolbar && 
      toolbar_slot->Toolbar->GetName())
    {
    char *clean_name = vtksys::SystemTools::RemoveChars(
      toolbar_slot->Toolbar->GetName(), " ");
    vtksys_stl::string key(clean_name);
    delete [] clean_name;
    
    key += "Visibility";
    if (this->GetApplication()->HasRegistryValue(
          2, "Toolbars", key.c_str()))
      {
      this->SetToolbarVisibility(
        toolbar_slot->Toolbar,
        this->GetApplication()->GetIntRegistryValue(
          2, "Toolbars", key.c_str()));
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SaveToolbarsVisibilityToRegistry()
{
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
        this->SaveToolbarVisibilityToRegistry((*it)->Toolbar);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::RestoreToolbarsVisibilityFromRegistry()
{
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
        this->RestoreToolbarVisibilityFromRegistry((*it)->Toolbar);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::PopulateToolbarsVisibilityMenu(vtkKWMenu *menu)
{
  if (this->Internals && menu)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    char help[500];
    for (; it != end; ++it)
      {
      if (*it && 
          (*it)->Toolbar && 
          (*it)->Toolbar->GetName() && 
          (*it)->Toolbar->IsCreated())
        {
        if (!menu->HasItem((*it)->Toolbar->GetName()))
          {
          vtksys_stl::string command("ToggleToolbarVisibility ");
          command += (*it)->Toolbar->GetTclName();

          sprintf(help, k_("Show/Hide the '%s' toolbar"), 
                  (*it)->Toolbar->GetName());
          int index = menu->AddCheckButton(
            (*it)->Toolbar->GetName(), this, command.c_str());
          menu->SetItemHelpString(index, help);
          }
        }
      }
    this->UpdateToolbarsVisibilityMenu(menu);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::UpdateToolbarsVisibilityMenu(vtkKWMenu *menu)
{
  if (this->Internals && menu)
    {
    vtkKWToolbarSetInternals::ToolbarsContainerIterator it = 
      this->Internals->Toolbars.begin();
    vtkKWToolbarSetInternals::ToolbarsContainerIterator end = 
      this->Internals->Toolbars.end();
    for (; it != end; ++it)
      {
      if (*it && (*it)->Toolbar && (*it)->Toolbar->GetName())
        {
        menu->SetItemSelectedState(
          (*it)->Toolbar->GetName(), (*it)->Visibility);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarVisibilityChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ToolbarVisibilityChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::InvokeToolbarVisibilityChangedCommand(
  vtkKWToolbar *toolbar)
{
  if (this->ToolbarVisibilityChangedCommand && 
      *this->ToolbarVisibilityChangedCommand && 
      this->GetApplication())
    {
    this->Script("%s %s",
                 this->ToolbarVisibilityChangedCommand, 
                 toolbar->GetTclName());
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetNumberOfToolbarsChangedCommand(
  vtkObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->NumberOfToolbarsChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::InvokeNumberOfToolbarsChangedCommand()
{
  this->InvokeObjectMethodCommand(this->NumberOfToolbarsChangedCommand);
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
        this->PropagateEnableState((*it)->Toolbar);
        this->PropagateEnableState((*it)->Separator);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ToolbarsFrame: " << this->ToolbarsFrame << endl;
  os << indent << "BottomSeparatorVisibility: " 
     << (this->BottomSeparatorVisibility ? "On" : "Off") << endl;
  os << indent << "TopSeparatorVisibility: " 
     << (this->TopSeparatorVisibility ? "On" : "Off") << endl;
  os << indent << "SynchronizeToolbarsVisibilityWithRegistry: " 
     << (this->SynchronizeToolbarsVisibilityWithRegistry ? "On" : "Off") 
     << endl;
}

