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

#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVApplicationSettingsInterface);
vtkCxxRevisionMacro(vtkPVApplicationSettingsInterface, "1.12");

int vtkPVApplicationSettingsInterfaceCommand(ClientData cd, Tcl_Interp *interp,
                                             int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVApplicationSettingsInterface::vtkPVApplicationSettingsInterface()
{
  // Interface settings

  this->ShowSourcesDescriptionCheckButton = 0;
  this->ShowSourcesNameCheckButton = 0;
  this->ShowTraceFilesCheckButton = 0;
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
  if (this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton->Delete();
    this->ShowTraceFilesCheckButton = NULL;
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
    "Show source names in browsers");
  this->ShowSourcesNameCheckButton->SetCommand(
    this, "ShowSourcesNameCallback");
  this->ShowSourcesNameCheckButton->SetBalloonHelpString(
    "This advanced option adjusts whether the unique source names "
    "are shown in the source browsers. This name is normally useful "
    "only to script developers.");

  tk_cmd << "pack " << this->ShowSourcesNameCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  if (!this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton = vtkKWCheckButton::New();
    }

  this->ShowTraceFilesCheckButton->SetParent(frame);
  this->ShowTraceFilesCheckButton->Create(app, 0);
  this->ShowTraceFilesCheckButton->SetText(
    "Show trace files on ParaView startup");
  this->ShowTraceFilesCheckButton->SetCommand(
    this, "ShowTraceFilesCallback");
  this->ShowTraceFilesCheckButton->SetBalloonHelpString(
    "When this advanced option is on, tracefiles will be detected and "
    "reported during startup. Turn this off to avoid unnecessary popup "
    "messages during startup.");

  if (!app->GetRegisteryValue(2,"RunTime", 
      VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY,0)||
    app->GetIntRegisteryValue(2,"RunTime",VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY))
    {
    this->ShowTraceFilesCheckButton->SetState(1);
    }
  else
    {
    this->ShowTraceFilesCheckButton->SetState(0);
    }

  tk_cmd << "pack " << this->ShowTraceFilesCheckButton->GetWidgetName()
    << "  -side top -anchor w -expand no -fill none" << endl;

  // --------------------------------------------------------------
  // Interface settings : show most recent panels

  // Not really supported by ParaView... (only in App Settings notebook)

  tk_cmd << "pack forget " 
         << this->ShowMostRecentPanelsCheckButton->GetWidgetName() << endl;

  // --------------------------------------------------------------
  // Interface settings : Drag & Drop

  // Not really supported by ParaView... (only in App Settings notebook)

  tk_cmd << "pack forget " 
         << this->DragAndDropFrame->GetWidgetName() << endl;

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

  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);

  // Interface settings : show sources description

  if (this->ShowSourcesDescriptionCheckButton && app)
    {
    this->ShowSourcesDescriptionCheckButton->SetState(
      app->GetShowSourcesLongHelp());
    }

  // Interface settings : show sources name

  if (this->ShowSourcesNameCheckButton && app)
    {
    this->ShowSourcesNameCheckButton->SetState(
      app->GetSourcesBrowserAlwaysShowName());
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesDescriptionCallback()
{
 if (!this->ShowSourcesDescriptionCheckButton ||
     !this->ShowSourcesDescriptionCheckButton->IsCreated())
   {
   return;
   }

 int flag = this->ShowSourcesDescriptionCheckButton->GetState() ? 1 : 0;

 this->GetApplication()->SetRegisteryValue(
   2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY, "%d", flag);

 vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
 if (app)
   {
   app->SetShowSourcesLongHelp(flag);
   }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowTraceFilesCallback()
{
  if (!this->ShowTraceFilesCheckButton ||
    !this->ShowTraceFilesCheckButton->IsCreated())
    {
    return;
    }

  int flag = this->ShowTraceFilesCheckButton->GetState() ? 1 : 0;

  this->GetApplication()->SetRegisteryValue(
    2, "RunTime", VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY, "%d", flag);

  vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
  if (app)
    {
    app->SetSourcesBrowserAlwaysShowName(flag);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::ShowSourcesNameCallback()
{
 if (!this->ShowSourcesNameCheckButton ||
     !this->ShowSourcesNameCheckButton->IsCreated())
   {
   return;
   }

 int flag = this->ShowSourcesNameCheckButton->GetState() ? 1 : 0;

 this->GetApplication()->SetRegisteryValue(
   2, "RunTime", VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY, "%d", flag);

 vtkPVApplication *app = vtkPVApplication::SafeDownCast(this->Application);
 if (app)
   {
   app->SetSourcesBrowserAlwaysShowName(flag);
   }
}

//----------------------------------------------------------------------------
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

  if (this->ShowTraceFilesCheckButton)
    {
    this->ShowTraceFilesCheckButton->SetEnabled(this->Enabled);
    }
}

//----------------------------------------------------------------------------
void vtkPVApplicationSettingsInterface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
