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
#include "vtkKWLabeledFrame.h"
#include "vtkKWToolbar.h"
#include "vtkObjectFactory.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"

//------------------------------------------------------------------------------

#define VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY "ShowSourcesLongHelp"
#define VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY       "SourcesBrowserAlwaysShowName"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkPVApplicationSettingsInterface, "1.3");

int vtkPVApplicationSettingsInterfaceCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::vtkPVApplicationSettingsInterface()
{
  this->Window = 0;

  // Interface settings

  this->ShowSourcesDescriptionCheckButton = 0;
  this->ShowSourcesNameCheckButton = 0;

  // Toolbar settings

  this->ToolbarSettingsFrame = 0;
  this->FlatFrameCheckButton = 0;
  this->FlatButtonsCheckButton = 0;
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

  // Toolbar settings

  if (this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame->Delete();
    this->ToolbarSettingsFrame = NULL;
    }

  if (this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->Delete();
    this->FlatFrameCheckButton = NULL;
    }

  if (this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->Delete();
    this->FlatButtonsCheckButton = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetWindow(vtkPVWindow *arg)
{
  if (this->Window == arg)
    {
    return;
    }
  this->Window = arg;
  this->Modified();
}

// ---------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::Create(vtkKWApplication *app)
{
  if (this->IsCreated())
    {
    vtkErrorMacro("The panel is already created.");
    return;
    }

  if (!this->Window)
    {
    vtkErrorMacro("A Window (vtkPVWindow) must be associated to the panel "
                  "before it is created.");
    return;
    }

  // Create the superclass instance (and set the application)

  this->Superclass::Create(app);

  ostrstream tk_cmd;
  vtkKWWidget *frame;

  // --------------------------------------------------------------
  // Interface settings : continuing...

  frame = this->InterfaceSettingsFrame->GetFrame();

  // Interface settings : show sources description

  if (!this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesDescriptionCheckButton->SetParent(frame);
  this->ShowSourcesDescriptionCheckButton->Create(app, 0);
  this->ShowSourcesDescriptionCheckButton->SetText("Show sources description");

  if (app->HasRegisteryValue(
        2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY))
    {
    this->ShowSourcesDescriptionCheckButton->SetState(
      app->GetIntRegisteryValue(
        2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY));
    }
  else
    {
    app->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", 1);
    this->ShowSourcesDescriptionCheckButton->SetState(1);
    }

  this->ShowSourcesDescriptionCheckButton->SetCommand(
    this, "ShowSourcesDescriptionCallback");

  this->ShowSourcesDescriptionCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the sources description "
    "are shown in the parameters page.");

  tk_cmd << "pack " << this->ShowSourcesDescriptionCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // Interface settings : show sources name

  if (!this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton = vtkKWCheckButton::New();
    }

  this->ShowSourcesNameCheckButton->SetParent(frame);
  this->ShowSourcesNameCheckButton->Create(app, 0);
  this->ShowSourcesNameCheckButton->SetText(
    "Show sources name in source browsers");

  if (app->HasRegisteryValue(
        2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY))
    {
    this->ShowSourcesNameCheckButton->SetState(
      app->GetIntRegisteryValue(
        2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY));
    }
  else
    {
    app->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", 0);
    this->ShowSourcesNameCheckButton->SetState(0);
    }

  this->ShowSourcesNameCheckButton->SetCommand(
    this, "ShowSourcesNameCallback");

  this->ShowSourcesNameCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the unique source names "
    "are shown in the source browsers. This name is normally useful "
    "only to script developers.");

  tk_cmd << "pack " << this->ShowSourcesNameCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Toolbar settings : main frame

  if (!this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame = vtkKWLabeledFrame::New();
    }

  this->ToolbarSettingsFrame->SetParent(
    this->GetPageWidget(VTK_KW_ASI_PREFERENCES_LABEL));
  this->ToolbarSettingsFrame->ShowHideFrameOn();
  this->ToolbarSettingsFrame->Create(app, 0);
  this->ToolbarSettingsFrame->SetLabel("Toolbar Settings");
    
  tk_cmd << "pack " << this->ToolbarSettingsFrame->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -padx 2 -pady 2" << endl;
  
  frame = this->ToolbarSettingsFrame->GetFrame();

  // Toolbar settings : flat frame

  if (!this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton = vtkKWCheckButton::New();
    }

  this->FlatFrameCheckButton->SetParent(frame);
  this->FlatFrameCheckButton->Create(app, 0);
  this->FlatFrameCheckButton->SetText("Flat frame");

  if (app->HasRegisteryValue(
        2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_FRAME_REG_KEY))
    {
    this->FlatFrameCheckButton->SetState(
      app->GetIntRegisteryValue(
        2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_FRAME_REG_KEY));
    }
  else
    {
    app->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_FRAME_REG_KEY, "%d",
      vtkKWToolbar::GetGlobalFlatAspect());
    this->FlatFrameCheckButton->SetState(
      vtkKWToolbar::GetGlobalFlatAspect());
    }

  this->FlatFrameCheckButton->SetCommand(
    this, "FlatFrameCallback");

  this->FlatFrameCheckButton->SetBalloonHelpString(
    "Display the toolbar frames using a flat aspect.");  

  tk_cmd << "pack " << this->FlatFrameCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // Toolbar settings : flat buttons

  if (!this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton = vtkKWCheckButton::New();
    }

  this->FlatButtonsCheckButton->SetParent(frame);
  this->FlatButtonsCheckButton->Create(app, 0);
  this->FlatButtonsCheckButton->SetText("Flat buttons");

  if (app->HasRegisteryValue(
        2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_BUTTONS_REG_KEY))
    {
    this->FlatButtonsCheckButton->SetState(
      app->GetIntRegisteryValue(
        2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_BUTTONS_REG_KEY));
    }
  else
    {
    app->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_BUTTONS_REG_KEY, "%d",
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    this->FlatButtonsCheckButton->SetState(
      vtkKWToolbar::GetGlobalWidgetsFlatAspect());
    }
  this->FlatButtonsCheckButton->SetCommand(
    this, "FlatButtonsCallback");

  this->FlatButtonsCheckButton->SetBalloonHelpString(
    "Display the toolbar buttons using a flat aspect.");  
  
  tk_cmd << "pack " << this->FlatButtonsCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetShowSourcesDescription(int v)
{
  if (this->IsCreated())
    {
    int flag = v ? 1 : 0;
    this->ShowSourcesDescriptionCheckButton->SetState(flag);
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", flag);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesDescriptionCallback()
{
 if (this->IsCreated())
   {
   int flag = this->ShowSourcesDescriptionCheckButton->GetState() ? 1 : 0;
   this->SetShowSourcesDescription(flag);
     
   if (this->Window)
     {
     this->Window->SetShowSourcesLongHelp(flag);
     }
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetShowSourcesName(int v)
{
  if (this->IsCreated())
    {
    int flag = v ? 1 : 0;
    this->ShowSourcesNameCheckButton->SetState(flag);
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", flag);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesNameCallback()
{
 if (this->IsCreated())
   {
   int flag = this->ShowSourcesNameCheckButton->GetState() ? 1 : 0;
   this->SetShowSourcesName(flag);

   if (this->Window && this->Window->GetMainView())
     {
     this->Window->GetMainView()->SetSourcesBrowserAlwaysShowName(flag);
     }
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetFlatFrame(int v)
{
  if (this->IsCreated())
    {
    int flag = v ? 1 : 0;
    this->FlatFrameCheckButton->SetState(flag);
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_FRAME_REG_KEY, "%d", flag);
    if (this->Window)
      {
      this->Window->UpdateToolbarAspect();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::FlatFrameCallback()
{
  if (this->IsCreated())
    {
    this->SetFlatFrame(this->FlatFrameCheckButton->GetState() ? 1 : 0);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetFlatButtons(int v)
{
  if (this->IsCreated())
    {
    int flag = v ? 1 : 0;
    this->FlatButtonsCheckButton->SetState(flag);
    this->GetApplication()->SetRegisteryValue(
      2, "RunTime", VTK_PV_ASI_TOOLBAR_FLAT_BUTTONS_REG_KEY, "%d", flag); 
    if (this->Window)
      {
      this->Window->UpdateToolbarAspect();
      }
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::FlatButtonsCallback()
{
  if (this->IsCreated())
    {
    this->SetFlatButtons(this->FlatButtonsCheckButton->GetState() ? 1 : 0);
    }
}

//------------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::SetEnabled(int e)
{
  this->Superclass::SetEnabled(e);

  // Interface settings

  if (this->ShowSourcesDescriptionCheckButton)
    {
    this->ShowSourcesDescriptionCheckButton->SetEnabled(e);
    }

  if (this->ShowSourcesNameCheckButton)
    {
    this->ShowSourcesNameCheckButton->SetEnabled(e);
    }

  // Toolbar settings

  if (this->ToolbarSettingsFrame)
    {
    this->ToolbarSettingsFrame->SetEnabled(e);
    }

  if (this->FlatFrameCheckButton)
    {
    this->FlatFrameCheckButton->SetEnabled(e);
    }

  if (this->FlatButtonsCheckButton)
    {
    this->FlatButtonsCheckButton->SetEnabled(e);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Window: " << this->Window << endl;

  // Toolbar settings

  os << indent << "ToolbarSettingsFrame: " 
     << this->ToolbarSettingsFrame << endl;
}
