/*=========================================================================

  Program:   ParaView
  Module:    vtkPVClipPlane.cxx
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
#include "vtkPVClipPlane.h"
#include "vtkStringList.h"
#include "vtkKWBoundsDisplay.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabel.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"

int vtkPVClipPlaneCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVClipPlane::vtkPVClipPlane()
{
  this->CommandFunction = vtkPVClipPlaneCommand;

  this->BoundsDisplay = vtkKWBoundsDisplay::New();

  this->CenterFrame = vtkKWWidget::New();
  this->CenterLabel = vtkKWLabel::New();
  this->CenterXEntry = vtkKWEntry::New();
  this->CenterYEntry = vtkKWEntry::New(); 
  this->CenterZEntry = vtkKWEntry::New();
  this->CenterResetButton = vtkKWPushButton::New();

  this->NormalFrame = vtkKWWidget::New();
  this->NormalLabel = vtkKWLabel::New();
  this->NormalXEntry = vtkKWEntry::New();
  this->NormalYEntry = vtkKWEntry::New(); 
  this->NormalZEntry = vtkKWEntry::New();
  this->NormalButtonFrame = vtkKWWidget::New();
  this->NormalCameraButton = vtkKWPushButton::New();
  this->NormalXButton = vtkKWPushButton::New();
  this->NormalYButton = vtkKWPushButton::New();
  this->NormalZButton = vtkKWPushButton::New();

  this->OffsetFrame = vtkKWWidget::New();
  this->OffsetLabel = vtkKWLabel::New();
  this->OffsetEntry = vtkKWEntry::New();

  this->ReplaceInputOn();
}

//----------------------------------------------------------------------------
vtkPVClipPlane::~vtkPVClipPlane()
{
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;

  this->CenterFrame->Delete();
  this->CenterFrame = NULL;
  this->CenterLabel->Delete();
  this->CenterLabel = NULL;
  this->CenterXEntry->Delete();
  this->CenterXEntry = NULL;
  this->CenterYEntry->Delete(); 
  this->CenterYEntry = NULL; 
  this->CenterZEntry->Delete();
  this->CenterZEntry = NULL;
  this->CenterResetButton->Delete();
  this->CenterResetButton = NULL;

  this->NormalFrame->Delete();
  this->NormalFrame = NULL;
  this->NormalLabel->Delete();
  this->NormalLabel = NULL;
  this->NormalXEntry->Delete();
  this->NormalXEntry = NULL;
  this->NormalYEntry->Delete(); 
  this->NormalYEntry = NULL; 
  this->NormalZEntry->Delete();
  this->NormalZEntry = NULL;
  this->NormalButtonFrame->Delete();
  this->NormalButtonFrame = NULL;
  this->NormalCameraButton->Delete();
  this->NormalCameraButton = NULL;
  this->NormalXButton->Delete();
  this->NormalXButton = NULL;
  this->NormalYButton->Delete();
  this->NormalYButton = NULL;
  this->NormalZButton->Delete();
  this->NormalZButton = NULL;

  this->OffsetFrame->Delete();
  this->OffsetFrame = NULL;
  this->OffsetLabel->Delete();
  this->OffsetLabel = NULL;
  this->OffsetEntry->Delete();
  this->OffsetEntry = NULL;
}

//----------------------------------------------------------------------------
vtkPVClipPlane* vtkPVClipPlane::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVClipPlane");
  if(ret)
    {
    return (vtkPVClipPlane*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVClipPlane;
}


//----------------------------------------------------------------------------
void vtkPVClipPlane::CreateProperties()
{
  vtkPVApplication* pvApp = this->GetPVApplication();
  
  this->vtkPVSource::CreateProperties();
 
  this->BoundsDisplay->SetParent(this->GetParameterFrame()->GetFrame());
  this->BoundsDisplay->Create(this->Application);
  this->BoundsDisplay->SetLabel("Input Bounds");
  this->Script("pack %s -side top -fill x",
               this->BoundsDisplay->GetWidgetName());

  this->CenterFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->CenterFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->CenterFrame->GetWidgetName());

  this->CenterLabel->SetParent(this->CenterFrame);
  this->CenterLabel->Create(this->Application, "-width 18 -justify right");
  this->CenterLabel->SetLabel("Center:");
  this->Script("pack %s -side left", this->CenterLabel->GetWidgetName());

  this->CenterXEntry->SetParent(this->CenterFrame);
  this->CenterXEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->CenterXEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->CenterXEntry->GetWidgetName());

  this->CenterYEntry->SetParent(this->CenterFrame);
  this->CenterYEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->CenterYEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->CenterYEntry->GetWidgetName());

  this->CenterZEntry->SetParent(this->CenterFrame);
  this->CenterZEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->CenterZEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->CenterZEntry->GetWidgetName());

  this->CenterResetButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->CenterResetButton->Create(this->Application, "");
  this->CenterResetButton->SetLabel("Set Plane Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("pack %s -side top -fill x -padx 2",
               this->CenterResetButton->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 0]", 
                                 this->CenterXEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetOrigin"); 
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 1]", 
                                 this->CenterYEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetOrigin"); 
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 2]", 
                                 this->CenterZEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetOrigin"); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), this->VTKSourceTclName, "SetOrigin",
                        this->CenterXEntry->GetTclName(),
                        this->CenterYEntry->GetTclName(), 
                        this->CenterZEntry->GetTclName());

  // Normal -------------------------
  this->NormalFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->NormalFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->NormalFrame->GetWidgetName());

  this->NormalLabel->SetParent(this->NormalFrame);
  this->NormalLabel->Create(this->Application, "-width 18 -justify right");
  this->NormalLabel->SetLabel("Normal:");
  this->Script("pack %s -side left", this->NormalLabel->GetWidgetName());

  this->NormalXEntry->SetParent(this->NormalFrame);
  this->NormalXEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->NormalXEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->NormalXEntry->GetWidgetName());

  this->NormalYEntry->SetParent(this->NormalFrame);
  this->NormalYEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->NormalYEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->NormalYEntry->GetWidgetName());

  this->NormalZEntry->SetParent(this->NormalFrame);
  this->NormalZEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->NormalZEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->NormalZEntry->GetWidgetName());

  this->NormalButtonFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->NormalButtonFrame->Create(this->Application, "frame", "");
  this->Script("pack %s -side top -fill x -padx 2",
               this->NormalButtonFrame->GetWidgetName());

  this->NormalCameraButton->SetParent(this->NormalButtonFrame);
  this->NormalCameraButton->Create(this->Application, "");
  this->NormalCameraButton->SetLabel("Use Camera Normal");
  this->NormalCameraButton->SetCommand(this, "NormalCameraCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalCameraButton->GetWidgetName());
  this->NormalXButton->SetParent(this->NormalButtonFrame);
  this->NormalXButton->Create(this->Application, "");
  this->NormalXButton->SetLabel("X Normal");
  this->NormalXButton->SetCommand(this, "NormalXCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalXButton->GetWidgetName());
  this->NormalYButton->SetParent(this->NormalButtonFrame);
  this->NormalYButton->Create(this->Application, "");
  this->NormalYButton->SetLabel("Y Normal");
  this->NormalYButton->SetCommand(this, "NormalYCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalYButton->GetWidgetName());
  this->NormalZButton->SetParent(this->NormalButtonFrame);
  this->NormalZButton->Create(this->Application, "");
  this->NormalZButton->SetLabel("Z Normal");
  this->NormalZButton->SetCommand(this, "NormalZCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalZButton->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 0]", 
                                 this->NormalXEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetNormal"); 
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 1]", 
                                 this->NormalYEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetNormal"); 
  this->ResetCommands->AddString("%s SetValue [lindex [%s %s] 2]", 
                                 this->NormalZEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetNormal"); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s \"[%s GetValue] [%s GetValue] [%s GetValue]\"",
                        this->GetTclName(), this->VTKSourceTclName, "SetNormal",
                        this->NormalXEntry->GetTclName(),
                        this->NormalYEntry->GetTclName(), 
                        this->NormalZEntry->GetTclName());


  // Offset -------------------------
  this->OffsetFrame->SetParent(this->GetParameterFrame()->GetFrame());
  this->OffsetFrame->Create(pvApp, "frame", "");
  this->Script("pack %s -side top -fill x",
               this->OffsetFrame->GetWidgetName());

  this->OffsetLabel->SetParent(this->OffsetFrame);
  this->OffsetLabel->Create(this->Application, "-width 18 -justify right");
  this->OffsetLabel->SetLabel("Offset:");
  this->Script("pack %s -side left", this->OffsetLabel->GetWidgetName());

  this->OffsetEntry->SetParent(this->OffsetFrame);
  this->OffsetEntry->Create(this->Application, "-width 2");
  this->Script("%s configure -xscrollcommand {%s EntryChanged}",
               this->OffsetEntry->GetWidgetName(), this->GetTclName());
  this->Script("pack %s -side left -fill x -expand t", 
               this->OffsetEntry->GetWidgetName());

  // Command to update the UI.
  this->ResetCommands->AddString("%s SetValue [%s %s]", 
                                 this->OffsetEntry->GetTclName(), 
                                 this->VTKSourceTclName, "GetOffset"); 
  // Format a command to move value from widget to vtkObjects (on all processes).
  // The VTK objects do not yet have to have the same Tcl name!
  this->AcceptCommands->AddString("%s AcceptHelper2 %s %s [%s GetValue]",
                        this->GetTclName(), this->VTKSourceTclName, "SetOffset",
                        this->OffsetEntry->GetTclName());

  this->UpdateProperties();
  this->UpdateParameterWidgets();
  this->CenterResetCallback();
}


//----------------------------------------------------------------------------
void vtkPVClipPlane::CenterResetCallback()
{
  vtkPVData *input;
  float bds[6];

  input = this->GetNthPVInput(0);
  if (input == NULL)
    {
    return;
    }
  input->GetBounds(bds);

  this->CenterXEntry->SetValue(0.5*(bds[0]+bds[1]), 3);
  this->CenterYEntry->SetValue(0.5*(bds[2]+bds[3]), 3);
  this->CenterZEntry->SetValue(0.5*(bds[4]+bds[5]), 3);
}


//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalCameraCallback()
{
  vtkKWView *view = this->GetView();
  vtkRenderer *ren;
  vtkCamera *cam;
  double normal[3];

  if (view == NULL)
    {
    vtkErrorMacro("Could not get the view/camera to set the normal.");
    return;
    }
  ren = vtkRenderer::SafeDownCast(view->GetViewport());
  if (ren == NULL)
    {
    vtkErrorMacro("Could not get the renderer/camera to set the normal.");
    return;
    }
  cam = ren->GetActiveCamera();
  if (cam == NULL)
    {
    vtkErrorMacro("Could not get the camera to set the normal.");
    return;
    }
  cam->GetViewPlaneNormal(normal);

  this->NormalXEntry->SetValue(-normal[0], 5);
  this->NormalYEntry->SetValue(-normal[1], 5);
  this->NormalZEntry->SetValue(-normal[2], 5);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalXCallback()
{
  this->NormalXEntry->SetValue(1.0, 1);
  this->NormalYEntry->SetValue(0.0, 1);
  this->NormalZEntry->SetValue(0.0, 1);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalYCallback()
{
  this->NormalXEntry->SetValue(0.0, 1);
  this->NormalYEntry->SetValue(1.0, 1);
  this->NormalZEntry->SetValue(0.0, 1);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalZCallback()
{
  this->NormalXEntry->SetValue(0.0, 1);
  this->NormalYEntry->SetValue(0.0, 1);
  this->NormalZEntry->SetValue(1.0, 1);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::UpdateParameterWidgets()
{
  vtkPVData *input;
  float bds[6];

  this->vtkPVSource::UpdateParameterWidgets();
  input = this->GetNthPVInput(0);
  if (input == NULL)
    {
    bds[0] = bds[2] = bds[4] = VTK_LARGE_FLOAT;
    bds[1] = bds[3] = bds[5] = -VTK_LARGE_FLOAT;
    }
  else
    {
    input->GetBounds(bds);
    }

  this->BoundsDisplay->SetBounds(bds);
}





