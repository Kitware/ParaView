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

#include "vtkKWToolbarSet.h"

#include "vtkKWApplication.h"
#include "vtkKWFrame.h"
#include "vtkKWToolbar.h"
#include "vtkLinkedList.txx"
#include "vtkLinkedListIterator.txx"
#include "vtkObjectFactory.h"

#if defined(_WIN32)
#define VTK_KW_TOOLBAR_RELIEF_SEP "groove"
#else
#define VTK_KW_TOOLBAR_RELIEF_SEP "sunken"
#endif

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkKWToolbarSet);
vtkCxxRevisionMacro(vtkKWToolbarSet, "1.3");

int vtkvtkKWToolbarSetCommand(ClientData cd, Tcl_Interp *interp,
                                  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWToolbarSet::vtkKWToolbarSet()
{
  this->ShowBottomSeparator  = 1;

  this->ToolbarsFrame        = vtkKWFrame::New();
  this->BottomSeparatorFrame = vtkKWFrame::New();

  this->Toolbars = vtkKWToolbarSet::ToolbarsContainer::New();
}

//----------------------------------------------------------------------------
vtkKWToolbarSet::~vtkKWToolbarSet()
{
  this->ToolbarsFrame->Delete();
  this->BottomSeparatorFrame->Delete();

  // Delete all toolbars

  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK)
      {
      if (toolbar_slot->SeparatorFrame)
        {
        toolbar_slot->SeparatorFrame->Delete();
        toolbar_slot->SeparatorFrame = NULL;
        }
      delete toolbar_slot;
      }
    it->GoToNextItem();
    }
  it->Delete();

  // Delete the container

  this->Toolbars->Delete();
}

//----------------------------------------------------------------------------
vtkKWToolbarSet::ToolbarSlot* 
vtkKWToolbarSet::GetToolbarSlot(vtkKWToolbar *toolbar)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarSlot *found = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK && 
        toolbar_slot->Toolbar == toolbar)
      {
      found = toolbar_slot;
      break;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return found;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::Create(vtkKWApplication *app, const char *args)
{
  // Set the application

  if (this->IsCreated())
    {
    vtkErrorMacro("The toolbar set is already created");
    return;
    }

  this->SetApplication(app);

  // Create the object

  this->Script("frame %s %s", this->GetWidgetName(), args ? args : "");

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
  if (!this->IsCreated())
    {
    return;
    }

  this->ToolbarsFrame->UnpackChildren();

  if (!this->GetNumberOfVisibleToolbars())
    {
    return;
    }

  ostrstream tk_cmd;

  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  vtkKWToolbar *previous = NULL;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK && 
        toolbar_slot->Toolbar && 
        toolbar_slot->Toolbar->IsCreated())
      {
      if (toolbar_slot->Visibility)
        {
        // Pack a separator

        if (previous)
          {
          if (!toolbar_slot->SeparatorFrame->IsCreated())
            {
            toolbar_slot->SeparatorFrame->SetParent(this->ToolbarsFrame);
            toolbar_slot->SeparatorFrame->Create(
              this->Application, "-width 2 -bd 1");
            this->Script("%s config -relief %s", 
                         toolbar_slot->SeparatorFrame->GetWidgetName(), 
                         VTK_KW_TOOLBAR_RELIEF_SEP);
            }
          tk_cmd << "pack " << toolbar_slot->SeparatorFrame->GetWidgetName() 
                 << " -side left -padx 1 -pady 0 -fill y -expand n" << endl;
          }
        previous = toolbar_slot->Toolbar;

        // Pack toolbar

        tk_cmd << "pack " << toolbar_slot->Toolbar->GetWidgetName() 
               << " -side left -padx 1 -pady 0 -fill both -expand "
               << (toolbar_slot->Toolbar->GetResizable() ? "y" : "n")
               << " -in " << this->ToolbarsFrame->GetWidgetName() << endl;
        }
      else
        {
        // Unpack separator and toolbar

        if (toolbar_slot->SeparatorFrame->IsCreated())
          {
          tk_cmd << "pack forget " 
                 << toolbar_slot->SeparatorFrame->GetWidgetName() << endl;
          }
        tk_cmd << "pack forget " 
               << toolbar_slot->Toolbar->GetWidgetName() << endl;
        }
      }
    it->GoToNextItem();
    }
  it->Delete();

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

  if (this->Toolbars->AppendItem(toolbar_slot) != VTK_OK)
    {
    vtkErrorMacro("Error while adding a toolbar to the set.");
    delete toolbar_slot;
    return 0;
    }
  
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
  return this->Toolbars->GetNumberOfItems();
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
vtkIdType vtkKWToolbarSet::GetNumberOfVisibleToolbars()
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  int nb = 0;

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK && toolbar_slot->Visibility)
      {
      nb++;
      }
    it->GoToNextItem();
    }
  it->Delete();

  return nb;
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarsFlatAspect(int f)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK && toolbar_slot->Toolbar)
      {
      toolbar_slot->Toolbar->SetFlatAspect(f);
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::SetToolbarsWidgetsFlatAspect(int f)
{
  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK && toolbar_slot->Toolbar)
      {
      toolbar_slot->Toolbar->SetWidgetsFlatAspect(f);
      }
    it->GoToNextItem();
    }
  it->Delete();
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
void vtkKWToolbarSet::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  vtkKWToolbarSet::ToolbarSlot *toolbar_slot = NULL;
  vtkKWToolbarSet::ToolbarsContainerIterator *it = 
    this->Toolbars->NewIterator();

  it->InitTraversal();
  while (!it->IsDoneWithTraversal())
    {
    if (it->GetData(toolbar_slot) == VTK_OK)
      {
      if (toolbar_slot->Toolbar)
        {
        toolbar_slot->Toolbar->SetEnabled(this->Enabled);
        }
      if (toolbar_slot->SeparatorFrame)
        {
        toolbar_slot->SeparatorFrame->SetEnabled(this->Enabled);
        }
      }
    it->GoToNextItem();
    }
  it->Delete();
}

//----------------------------------------------------------------------------
void vtkKWToolbarSet::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "ToolbarsFrame: " << this->ToolbarsFrame << endl;
  os << indent << "ShowBottomSeparator: " 
     << (this->ShowBottomSeparator ? "On" : "Off") << endl;
}

