/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWApplicationSettingsInterface.cxx
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

#include "vtkKWApplicationSettingsInterface.h"

#include "vtkKWApplication.h"
#include "vtkKWCheckButton.h"
#include "vtkKWIcon.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWWindow.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkKWApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkKWApplicationSettingsInterface, "1.3");

int vtkKWApplicationSettingsInterfaceCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface::vtkKWApplicationSettingsInterface()
{
  // Interface settings

  this->InterfaceSettingsFrame = 0;
  this->ConfirmExitCheckButton = 0;
  this->SaveGeometryCheckButton = 0;
  this->ShowSplashScreenCheckButton = 0;
  this->ShowBalloonHelpCheckButton = 0;
}

//----------------------------------------------------------------------------
vtkKWApplicationSettingsInterface::~vtkKWApplicationSettingsInterface()
{
  // Interface settings

  if (this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame->Delete();
    this->InterfaceSettingsFrame = NULL;
    }

  if (this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton->Delete();
    this->ConfirmExitCheckButton = NULL;
    }

  if (this->SaveGeometryCheckButton)
    {
    this->SaveGeometryCheckButton->Delete();
    this->SaveGeometryCheckButton = NULL;
    }

  if (this->ShowSplashScreenCheckButton)
    {
    this->ShowSplashScreenCheckButton->Delete();
    this->ShowSplashScreenCheckButton = NULL;
    }

  if (this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton->Delete();
    this->ShowBalloonHelpCheckButton = NULL;
    }
}

// ---------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::Create(vtkKWApplication *app)
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
  // Add a "Preferences" page

  vtkKWIcon *ico = vtkKWIcon::New();
  ico->SetImageData(vtkKWIcon::ICON_PREFERENCES);
  this->AddPage(VTK_KW_ASI_PREFERENCES_LABEL, 
                "Set the application preferences", 
                ico);
  ico->Delete();
  
  // --------------------------------------------------------------
  // Interface settings : main frame

  if (!this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame = vtkKWLabeledFrame::New();
    }

  this->InterfaceSettingsFrame->SetParent(
    this->GetPageWidget(VTK_KW_ASI_PREFERENCES_LABEL));
  this->InterfaceSettingsFrame->ShowHideFrameOn();
  this->InterfaceSettingsFrame->Create(app, 0);
  this->InterfaceSettingsFrame->SetLabel("Interface Settings");
    
  tk_cmd << "pack " << this->InterfaceSettingsFrame->GetWidgetName()
         << " -side top -anchor w -expand y -fill x -padx 2 -pady 2" << endl;
  
  frame = this->InterfaceSettingsFrame->GetFrame();

  // Interface settings : Confirm on exit ?

  if (!this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton = vtkKWCheckButton::New();
    }

  this->ConfirmExitCheckButton->SetParent(frame);
  this->ConfirmExitCheckButton->Create(app, 0);
  this->ConfirmExitCheckButton->SetText("Confirm on exit");

  int res = app->GetMessageDialogResponse(VTK_KW_EXIT_DIALOG_NAME);
  this->ConfirmExitCheckButton->SetState(res ? 0 : 1);

  this->ConfirmExitCheckButton->SetCommand(
    this, "ConfirmExitCheckButtonCallback");

  this->ConfirmExitCheckButton->SetBalloonHelpString(
    "A confirmation dialog will be presented to the user on exit.");

  tk_cmd << "pack " << this->ConfirmExitCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // Interface settings : Save application geometry on exit ?

  if (!this->SaveGeometryCheckButton)
    {
    this->SaveGeometryCheckButton = vtkKWCheckButton::New();
    }

  this->SaveGeometryCheckButton->SetParent(frame);
  this->SaveGeometryCheckButton->Create(app, 0);
  this->SaveGeometryCheckButton->SetText("Save window geometry on exit");
  
  if (app->HasRegisteryValue(
        2, "Geometry", VTK_KW_ASI_SAVE_WINDOW_GEOMETRY_REG_KEY))
    {
    this->SaveGeometryCheckButton->SetState(
      app->GetIntRegisteryValue(
        2, "Geometry", VTK_KW_ASI_SAVE_WINDOW_GEOMETRY_REG_KEY));
    }
  else
    {
    app->SetRegisteryValue(
      2, "Geometry", VTK_KW_ASI_SAVE_WINDOW_GEOMETRY_REG_KEY, "%d", 1);
    this->SaveGeometryCheckButton->SetState(1);
    }
  
  this->SaveGeometryCheckButton->SetCommand(
    this, "SaveGeometryCheckButtonCallback");

  this->SaveGeometryCheckButton->SetBalloonHelpString(
    "Save the window size and location on exit and restore it on startup.");

  tk_cmd << "pack " << this->SaveGeometryCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // Interface settings : Show splash screen ?

  if (app->GetHasSplashScreen())
    {
    if (!this->ShowSplashScreenCheckButton)
      {
      this->ShowSplashScreenCheckButton = vtkKWCheckButton::New();
      }

    this->ShowSplashScreenCheckButton->SetParent(frame);
    this->ShowSplashScreenCheckButton->Create(app, 0);
    this->ShowSplashScreenCheckButton->SetText("Show splash screen");

    if (app->HasRegisteryValue(
          2, "RunTime", VTK_KW_ASI_SHOW_SPLASH_SCREEN_REG_KEY))
      {
      this->ShowSplashScreenCheckButton->SetState(
        app->GetIntRegisteryValue(
          2, "RunTime", VTK_KW_ASI_SHOW_SPLASH_SCREEN_REG_KEY));
      }
    else
      {
      this->ShowSplashScreenCheckButton->SetState(
        app->GetShowSplashScreen());
      }

    this->ShowSplashScreenCheckButton->SetCommand(
      this, "ShowSplashScreenCheckButtonCallback");

    this->ShowSplashScreenCheckButton->SetBalloonHelpString(
      "Display the splash information screen at startup.");

    tk_cmd << "pack " << this->ShowSplashScreenCheckButton->GetWidgetName()
           << "  -side top -anchor w -expand no -fill none" << endl;
    }

  // Interface settings : Show balloon help ?

  if (!this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton = vtkKWCheckButton::New();
    }

  this->ShowBalloonHelpCheckButton->SetParent(frame);
  this->ShowBalloonHelpCheckButton->Create(app, 0);
  this->ShowBalloonHelpCheckButton->SetText("Show tooltips");

  if (app->HasRegisteryValue(
        2, "RunTime", VTK_KW_ASI_SHOW_TOOLTIPS_REG_KEY))
    {
    this->ShowBalloonHelpCheckButton->SetState(
      app->GetIntRegisteryValue(
        2, "RunTime", VTK_KW_ASI_SHOW_TOOLTIPS_REG_KEY));
    }
  else
    {
    this->ShowBalloonHelpCheckButton->SetState(
      app->GetShowBalloonHelp());
    }

  this->ShowBalloonHelpCheckButton->SetCommand(
    this, "ShowBalloonHelpCheckButtonCallback");

  this->ShowBalloonHelpCheckButton->SetBalloonHelpString(
    "Display help in a yellow popup-box on the screen when you rest the "
    "mouse over an item that supports it.");

  tk_cmd << "pack " << this->ShowBalloonHelpCheckButton->GetWidgetName()
         << "  -side top -anchor w -expand no -fill none" << endl;

  // Pack 

  tk_cmd << ends;
  this->Script(tk_cmd.str());
  tk_cmd.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SetConfirmExit(int v)
{
  if (this->IsCreated())
    {
    int flag = v ? 1 : 0;
    this->ConfirmExitCheckButton->SetState(flag);
    this->GetApplication()->SetMessageDialogResponse(
      VTK_KW_EXIT_DIALOG_NAME, flag ? 0 : 1);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ConfirmExitCheckButtonCallback()
{
 if (this->IsCreated())
   {
   this->SetConfirmExit(this->ConfirmExitCheckButton->GetState() ? 1 : 0);
   }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SetSaveGeometry(int v)
{
 if (this->IsCreated())
   {
   int flag = v ? 1 : 0;
   this->SaveGeometryCheckButton->SetState(flag);
   this->GetApplication()->SetRegisteryValue(
     2, "Geometry", VTK_KW_ASI_SAVE_WINDOW_GEOMETRY_REG_KEY, "%d", flag);
   }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SaveGeometryCheckButtonCallback()
{
 if (this->IsCreated())
   {
   this->SetSaveGeometry(this->SaveGeometryCheckButton->GetState() ? 1 : 0);
   }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SetShowSplashScreen(int v)
{
 if (this->IsCreated())
   {
   int flag = v ? 1 : 0;
   this->ShowSplashScreenCheckButton->SetState(flag);
   this->GetApplication()->SetRegisteryValue(
     2, "RunTime", VTK_KW_ASI_SHOW_SPLASH_SCREEN_REG_KEY, "%d", flag);
   this->GetApplication()->SetShowSplashScreen(flag);
   }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ShowSplashScreenCheckButtonCallback()
{
 if (this->IsCreated())
   {
   this->SetShowSplashScreen(
     this->ShowSplashScreenCheckButton->GetState() ? 1 : 0);
   }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SetShowBalloonHelp(int v)
{
 if (this->IsCreated())
   {
   int flag = v ? 1 : 0;
   this->ShowBalloonHelpCheckButton->SetState(flag);
   this->GetApplication()->SetRegisteryValue(
     2, "RunTime", VTK_KW_ASI_SHOW_TOOLTIPS_REG_KEY, "%d", flag);
   this->GetApplication()->SetShowBalloonHelp(flag);
   }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::ShowBalloonHelpCheckButtonCallback()
{
  if (this->IsCreated())
   {
   this->SetShowBalloonHelp(
     this->ShowBalloonHelpCheckButton->GetState() ? 1 : 0);
   }
}

//------------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::SetEnabled(int e)
{
  if (this->InterfaceSettingsFrame)
    {
    this->InterfaceSettingsFrame->SetEnabled(e);
    }

  if (this->ConfirmExitCheckButton)
    {
    this->ConfirmExitCheckButton->SetEnabled(e);
    }

  if (this->SaveGeometryCheckButton)
    {
    this->SaveGeometryCheckButton->SetEnabled(e);
    }

  if (this->ShowSplashScreenCheckButton)
    {
    this->ShowSplashScreenCheckButton->SetEnabled(e);
    }

  if (this->ShowBalloonHelpCheckButton)
    {
    this->ShowBalloonHelpCheckButton->SetEnabled(e);
    }
}

//----------------------------------------------------------------------------
void vtkKWApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InterfaceSettingsFrame: " 
     << this->InterfaceSettingsFrame << endl;
}
