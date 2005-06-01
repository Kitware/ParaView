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
#include "vtkKWMenu.h"
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"
#include "vtkKWTkUtilities.h"

#if defined(_WIN32)
#define VTK_KW_TOOLBAR_RELIEF_SEP "groove"
#else
#define VTK_KW_TOOLBAR_RELIEF_SEP "sunken"
#endif

#include <kwsys/stl/list>
#include <kwsys/SystemTools.hxx>

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWToolbarSet);
vtkCxxRevisionMacro(vtkKWToolbarSet, "1.16");

int vtkvtkKWToolbarSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
class vtkKWToolbarSetInternals
{
public:

  typedef kwsys_stl::list<vtkKWToolbarSet::ToolbarSlot*> ToolbarsContainer;
  typedef kwsys_stl::list<vtkKWToolbarSet::ToolbarSlot*>::iterator ToolbarsContainerIterator;

  ToolbarsContainer Toolbars;

  kwsys_stl::string PreviousPackInfo;
  kwsys_stl::string PreviousGridInfo;
};

//----------------------------------------------------------------------------
vtkKWToolbarSet::vtkKWToolbarSet()
{
  this->ShowBottomSeparator  = 0;
  this->ShowTopSeparator  = 0;
  this->SynchronizeToolbarsVisibilityWithRegistry  = 0;

  this->ToolbarVisibilityChangedCommand  = NULL;
  this->NumberOfToolbarsChangedCommand  = NULL;

  this->ToolbarsFrame        = vtkKWFrame::New();
  this->BottomSeparatorFrame = vtkKWFrame::New();
  this->TopSeparatorFrame = vtkKWFrame::New();

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

  if (this->BottomSeparatorFrame)
    {
    this->BottomSeparatorFrame->Delete();
    this->BottomSeparatorFrame = NULL;
    }

  if (this->TopSeparatorFrame)
    {
    this->TopSeparatorFrame->Delete();
    this->TopSeparatorFrame = NULL;
    }

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
  this->ToolbarsFrame->Create(app, NULL);

  // Bottom separator

  this->BottomSeparatorFrame->SetParent(this);  
  this->BottomSeparatorFrame->Create(app, "-height 2 -bd 1");

  this->Script("%s config -relief %s", 
               this->BottomSeparatorFrame->GetWidgetName(), 
               VTK_KW_TOOLBAR_RELIEF_SEP);

  // Top separator

  this->TopSeparatorFrame->SetParent(this);  
  this->TopSeparatorFrame->Create(app, "-height 2 -bd 1");

  this->Script("%s config -relief %s", 
               this->TopSeparatorFrame->GetWidgetName(), 
               VTK_KW_TOOLBAR_RELIEF_SEP);

  // This is needed for the hack in Pack() to work, otherwise
  // the widget is not hidden properly if the user packs it manually
  // and don't update it after that (i.e. call a method that would
  // call Pack()). This will likely force Pack() to be called
  // twice each time we add a toolbar, but it's not like we have
  // thousand's of them. Again, hack, sorry. 
  
  this->SetBind(this, "<Map>", "Pack");

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
  // should manually configure the height to be 1 (sadly, it can not be 0:
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
        this->Script("pack forget %s", this->GetWidgetName());
        this->Internals->PreviousGridInfo.clear();
        }
      else
        {
        kwsys_stl::string grid_info = 
          this->Script("grid info %s", this->GetWidgetName());
        if (!grid_info.empty())
          {
          this->Internals->PreviousPackInfo.clear();
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
        this->Internals->PreviousPackInfo.clear();
        this->Internals->PreviousGridInfo.clear();
        }
      else if (!this->Internals->PreviousGridInfo.empty())
        {
        this->Script("grid %s %s", this->GetWidgetName(), 
                     this->Internals->PreviousGridInfo.c_str());
        this->Internals->PreviousPackInfo.clear();
        this->Internals->PreviousGridInfo.clear();
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

  if (this->ShowBottomSeparator && this->GetNumberOfVisibleToolbars())
    {
    this->Script(
      "pack %s -side top -fill x -expand y -padx 0 -pady 2 -after",
      this->BottomSeparatorFrame->GetWidgetName(),
      this->ToolbarsFrame->GetWidgetName());
    }
  else
    {
    this->BottomSeparatorFrame->Unpack();
    }
}

// ----------------------------------------------------------------------------
void vtkKWToolbarSet::PackTopSeparator()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->ShowTopSeparator && this->GetNumberOfVisibleToolbars())
    {
    this->Script(
      "pack %s -side top -fill x -expand y -padx 0 -pady 2 -before %s",
      this->TopSeparatorFrame->GetWidgetName(),
      this->ToolbarsFrame->GetWidgetName());
    }
  else
    {
    this->TopSeparatorFrame->Unpack();
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

  toolbar_slot->SeparatorFrame = vtkKWFrame::New();
  this->PropagateEnableState(toolbar_slot->SeparatorFrame);

  toolbar_slot->Visibility = default_visibility;
  if (this->SynchronizeToolbarsVisibilityWithRegistry)
    {
    this->RestoreToolbarVisibilityFromRegistry(toolbar_slot->Toolbar);
    }

  // Pack the toolbars if we just brought a visible toolbar

  if (toolbar_slot->Visibility)
    {
    this->Pack();
    }

  this->InvokeNumberOfToolbarsChangedCommand();

  return 1;
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
    this->InvokeToolbarVisibilityChangedCommand();
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
void vtkKWToolbarSet::SetShowTopSeparator(int arg)
{
  if (this->ShowTopSeparator == arg)
    {
    return;
    }

  this->ShowTopSeparator = arg;
  this->Modified();

  this->PackTopSeparator();
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
void vtkKWToolbarSet::SaveToolbarVisibilityToRegistry(
  vtkKWToolbar *toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = this->GetToolbarSlot(toolbar);
  if (toolbar_slot && 
      toolbar_slot->Toolbar && 
      toolbar_slot->Toolbar->GetName())
    {
    char *clean_name = kwsys::SystemTools::RemoveChars(
      toolbar_slot->Toolbar->GetName(), " ");
    kwsys_stl::string key(clean_name);
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
    char *clean_name = kwsys::SystemTools::RemoveChars(
      toolbar_slot->Toolbar->GetName(), " ");
    kwsys_stl::string key(clean_name);
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
    for (; it != end; ++it)
      {
      if (*it && 
          (*it)->Toolbar && 
          (*it)->Toolbar->GetName() && 
          (*it)->Toolbar->IsCreated())
        {
        if (!menu->HasItem((*it)->Toolbar->GetName()))
          {
          char *rbv = menu->CreateCheckButtonVariable(
            menu, (*it)->Toolbar->GetName());

          kwsys_stl::string command("ToggleToolbarVisibility ");
          command += (*it)->Toolbar->GetTclName();

          kwsys_stl::string help("Show/Hide the ");
          help += (*it)->Toolbar->GetName();
          help += " toolbar";
        
          menu->AddCheckButton(
            (*it)->Toolbar->GetName(), rbv, 
            this, command.c_str(), help.c_str());
          delete [] rbv;
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
        menu->CheckCheckButton(
          menu, (*it)->Toolbar->GetName(), (*it)->Visibility);
        }
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::InvokeToolbarVisibilityChangedCommand()
{
  if (this->ToolbarVisibilityChangedCommand && 
      *this->ToolbarVisibilityChangedCommand)
    {
    this->Script("eval %s", this->ToolbarVisibilityChangedCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarVisibilityChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->ToolbarVisibilityChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::InvokeNumberOfToolbarsChangedCommand()
{
  if (this->NumberOfToolbarsChangedCommand && 
      *this->NumberOfToolbarsChangedCommand)
    {
    this->Script("eval %s", this->NumberOfToolbarsChangedCommand);
    }
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetNumberOfToolbarsChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->NumberOfToolbarsChangedCommand, object, method);
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
        this->PropagateEnableState((*it)->SeparatorFrame);
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
  os << indent << "ShowTopSeparator: " 
     << (this->ShowTopSeparator ? "On" : "Off") << endl;
  os << indent << "SynchronizeToolbarsVisibilityWithRegistry: " 
     << (this->SynchronizeToolbarsVisibilityWithRegistry ? "On" : "Off") 
     << endl;
}

