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
#include "vtkKWScalarComponentSelectionWidget.h"

#include "vtkKWEvent.h"
#include "vtkKWLabeledOptionMenu.h"
#include "vtkKWOptionMenu.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkKWScalarComponentSelectionWidget, "1.1.2.1");
vtkStandardNewMacro(vtkKWScalarComponentSelectionWidget);

//----------------------------------------------------------------------------
vtkKWScalarComponentSelectionWidget::vtkKWScalarComponentSelectionWidget()
{
  this->IndependentComponents           = 1;
  this->NumberOfComponents              = VTK_MAX_VRCOMP;
  this->SelectedComponent               = 0;
  this->AllowComponentSelection         = 1;

  this->SelectedComponentChangedCommand = NULL;

  // GUI

  this->SelectedComponentOptionMenu     = vtkKWLabeledOptionMenu::New();
}

//----------------------------------------------------------------------------
vtkKWScalarComponentSelectionWidget::~vtkKWScalarComponentSelectionWidget()
{
  // Commands

  if (this->SelectedComponentChangedCommand)
    {
    delete [] this->SelectedComponentChangedCommand;
    this->SelectedComponentChangedCommand = NULL;
    }

  // GUI

  if (this->SelectedComponentOptionMenu)
    {
    this->SelectedComponentOptionMenu->Delete();
    this->SelectedComponentOptionMenu = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::Create(
  vtkKWApplication *app, char *args)
{
  // Check if already created

  if (this->IsCreated())
    {
    vtkErrorMacro("KWScalarComponentSelectionWidget already created");
    return;
    }

  this->SetApplication(app);

  // Create the top level

  const char *wname = this->GetWidgetName();
  this->Script("frame %s -bd 0 -relief flat %s", wname, (args ? args : ""));

  // --------------------------------------------------------------
  // Component selection

  this->SelectedComponentOptionMenu->SetParent(this);
  this->SelectedComponentOptionMenu->Create(app);
  this->SelectedComponentOptionMenu->SetLabel("Component:");
  this->SelectedComponentOptionMenu->SetBalloonHelpString(
    "Select the component this interface will control.");

  // Pack

  this->Pack();

  // Update

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::Pack()
{
  if (!this->IsCreated())
    {
    return;
    }

  if (this->AllowComponentSelection)
    {
    this->Script("pack %s -side top -padx 0 -pady 0 -anchor w",
                 this->SelectedComponentOptionMenu->GetWidgetName());
    }
  else
    {
    this->Script("pack forget %s", 
                 this->SelectedComponentOptionMenu->GetWidgetName());
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::Update()
{
  // Update enable state

  this->UpdateEnableState();

  // In the dependent case, everything is in the component 0

  if (!this->IndependentComponents ||
      (this->SelectedComponent < 0 ||
       this->SelectedComponent >= this->NumberOfComponents))
    {
    this->SelectedComponent = 0;
    }

  int i;

  // Component selection menu

  if (this->SelectedComponentOptionMenu)
    {
    vtkKWOptionMenu *menu = 
      this->SelectedComponentOptionMenu->GetOptionMenu();

    if (this->SelectedComponentOptionMenu->IsCreated() &&
        menu->GetNumberOfEntries() != this->NumberOfComponents)
      {
      menu->ClearEntries();
      for (i = 0; i < this->NumberOfComponents; ++i)
        {
        ostrstream cmd_name, cmd_method;
        
        cmd_name << i + 1 << ends;
        cmd_method << "SelectedComponentCallback " << i << ends;

        menu->AddEntryWithCommand(cmd_name.str(), this, cmd_method.str());

        cmd_name.rdbuf()->freeze(0);
        cmd_method.rdbuf()->freeze(0);
        }
      }
    
    if (menu->GetNumberOfEntries() && this->IndependentComponents)
      {
      ostrstream v;
      v << this->SelectedComponent + 1 << ends;
      menu->SetValue(v.str());
      v.rdbuf()->freeze(0);
      }
    else
      {
      menu->SetValue("");
      }

    if (!this->IndependentComponents || this->NumberOfComponents <= 1)
      {
      this->SelectedComponentOptionMenu->SetEnabled(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  if (this->SelectedComponentOptionMenu)
    {
    this->SelectedComponentOptionMenu->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetAllowComponentSelection(
  int arg)
{
  if (this->AllowComponentSelection == arg)
    {
    return;
    }

  this->AllowComponentSelection = arg;
  this->Modified();

  this->Pack();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetIndependentComponents(int arg)
{
  if (this->IndependentComponents == arg)
    {
    return;
    }

  this->IndependentComponents = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetSelectedComponent(int arg)
{
  if (this->SelectedComponent == arg ||
      arg < 0 || arg >= this->NumberOfComponents)
    {
    return;
    }

  this->SelectedComponent = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetNumberOfComponents(int arg)
{
  if (this->NumberOfComponents == arg ||
      arg < 1 || arg > VTK_MAX_VRCOMP)
    {
    return;
    }

  this->NumberOfComponents = arg;
  this->Modified();

  this->Update();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::InvokeSelectedComponentChangedCommand()
{
  if (this->SelectedComponentChangedCommand && 
      *this->SelectedComponentChangedCommand)
    {
    this->Script("eval %s %d", 
                 this->SelectedComponentChangedCommand, 
                 this->SelectedComponent);
    }

  this->InvokeEvent(vtkKWEvent::ScalarComponentChangedEvent, 
                    &this->SelectedComponent);
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SetSelectedComponentChangedCommand(
  vtkKWObject *object, const char *method)
{
  this->SetObjectMethodCommand(
    &this->SelectedComponentChangedCommand, object, method);
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::SelectedComponentCallback(int n)
{
  if (this->SelectedComponent == n)
    {
    return;
    }

  this->SelectedComponent = n;
  this->Update();
  this->InvokeSelectedComponentChangedCommand();
}

//----------------------------------------------------------------------------
void vtkKWScalarComponentSelectionWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  
  os << indent << "IndependentComponents: "
     << (this->IndependentComponents ? "On" : "Off") << endl;
  os << indent << "SelectedComponent: " 
     << this->SelectedComponent << endl;
  os << indent << "NumberOfComponents: " 
     << this->NumberOfComponents << endl;
  os << indent << "AllowComponentSelection: "
     << (this->AllowComponentSelection ? "On" : "Off") << endl;
  os << indent << "SelectedComponentOptionMenu: " 
     << this->SelectedComponentOptionMenu << endl;
}
