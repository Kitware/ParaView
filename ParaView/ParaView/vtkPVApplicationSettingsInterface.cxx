/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVApplicationSettingsInterface.cxx
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

#include "vtkPVApplicationSettingsInterface.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"

//------------------------------------------------------------------------------

#define VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY "ShowSourcesLongHelp"
#define VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY       "SourcesBrowserAlwaysShowName"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkPVApplicationSettingsInterface, "1.8");

int vtkPVApplicationSettingsInterfaceCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::vtkPVApplicationSettingsInterface()
{
  // Interface settings

  this->ShowSourcesDescriptionCheckButton = 0;
  this->ShowSourcesNameCheckButton = 0;
}

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::~vtkPVApplicationSettingsInterface()
{
  this->SetWindow(NULL);

  // Interface settings

  if (this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton->Delete();
    this->ShowSourcesDescriptionCheckButton = NULL;
    }

  if (this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton->Delete();
    this->ShowSourcesNameCheckButton = NULL;
    }
}

// ---------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create(app);

  ostrstream tk_cmd;
  vtkKWWidget *frame;

  // --------------------------------------------------------------
  // Interface settings : continuing...

  frame = this->InterfaceSettingsFrame->GetFrame();

  // --------------------------------------------------------------
  // Interface settings : show sources description

  if (!this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesDescriptionCheckButton->SetParent(frame);
  this->ShowSourcesDescriptionCheckButton->Create(app, 0);
  this->ShowSourcesDescriptionCheckButton->SetText("Show sources description");
  this->ShowSourcesDescriptionCheckButton->SetCommand(
    this, "ShowSourcesDescriptionCallback");
  this->ShowSourcesDescriptionCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the sources description "
    "are shown in the parameters page.");

  tk_cmd << "pack " << this->ShowSourcesDescriptionCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : show sources name

  if (!this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesNameCheckButton->SetParent(frame);
  this->ShowSourcesNameCheckButton->Create(app, 0);
  this->ShowSourcesNameCheckButton->SetText(
    "Show sources name in source browsers");
  this->ShowSourcesNameCheckButton->SetCommand(
    this, "ShowSourcesNameCallback");
  this->ShowSourcesNameCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the unique source names "
    "are shown in the source browsers. This name is normally useful "
    "only to script developers.");

  tk_cmd << "pack " << this->ShowSourcesNameCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);

  // Update according to the current Window

  this->Update();
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::Update()
{
  this->Superclass::Update();

  if (!this->IsCreated() || !this->Window)
    {
    return;
    }

  // Interface settings : show sources description

  if (this->ShowSourcesDescriptionCheckButton)
    {
    if (this->Application->HasRegisteryValue(
          2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY))
      {
      this->ShowSourcesDescriptionCheckButton->SetState(
        this->Application->GetIntRegisteryValue(
          2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY));
      }
    else
      {
      this->Application->SetRegisteryValue(
        2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", 1);
      this->ShowSourcesDescriptionCheckButton->SetState(1);
      }
    }

  // Interface settings : show sources name

  if (this->ShowSourcesNameCheckButton)
    {
    if (this->Application->HasRegisteryValue(
          2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY))
      {
      this->ShowSourcesNameCheckButton->SetState(
        this->Application->GetIntRegisteryValue(
          2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY));
      }
    else
      {
      this->Application->SetRegisteryValue(
        2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", 0);
      this->ShowSourcesNameCheckButton->SetState(0);
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesDescriptionCallback()
{
 if (this->IsCreated())
   {
   int flag = this->ShowSourcesDescriptionCheckButton->GetState() ? 1 : 0;
   this->GetApplication()->SetRegisteryValue(
     2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", flag);
   
   vtkPVWindow *win = vtkPVWindow::SafeDownCast(this->Window);
   if (win)
     {
     win->SetShowSourcesLongHelp(flag);
     }
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesNameCallback()
{
 if (this->IsCreated())
   {
   int flag = this->ShowSourcesNameCheckButton->GetState() ? 1 : 0;
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", flag);

   vtkPVWindow *win = vtkPVWindow::SafeDownCast(this->Window);
   if (win && win->GetMainView())
     {
     win->GetMainView()->SetSourcesBrowserAlwaysShowName(flag);
     }
   }
}

//------------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  // Interface settings

  if (this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton->SetEnabled(this->Enabled);
    }

  if (this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
