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
#include "vtkPVVectorEntry.h"

int vtkPVClipPlaneCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVClipPlane::vtkPVClipPlane()
{
  this->CommandFunction = vtkPVClipPlaneCommand;

  this->BoundsDisplay = vtkKWBoundsDisplay::New();

  this->CenterEntry = vtkPVVectorEntry::New();
  this->CenterResetButton = vtkKWPushButton::New();

  this->NormalEntry = vtkPVVectorEntry::New();
  this->NormalButtonFrame = vtkKWWidget::New();
  this->NormalCameraButton = vtkKWPushButton::New();
  this->NormalXButton = vtkKWPushButton::New();
  this->NormalYButton = vtkKWPushButton::New();
  this->NormalZButton = vtkKWPushButton::New();

  this->OffsetEntry = vtkPVVectorEntry::New();

  this->ReplaceInputOn();
}

//----------------------------------------------------------------------------
vtkPVClipPlane::~vtkPVClipPlane()
{
  this->BoundsDisplay->Delete();
  this->BoundsDisplay = NULL;

  this->CenterEntry->Delete();
  this->CenterEntry = NULL;
  this->CenterResetButton->Delete();
  this->CenterResetButton = NULL;

  this->NormalEntry->Delete();
  this->NormalEntry = NULL;
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

  this->CenterEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->CenterEntry->SetPVSource(this);
  this->CenterEntry->Create(this->Application, "Center:", 3, NULL, "SetOrigin",
                            "GetOrigin", NULL, this->GetVTKSourceTclName());
  this->Widgets->AddItem(this->CenterEntry);
  this->Script("pack %s -side top -fill x",
               this->CenterEntry->GetWidgetName());

  this->CenterResetButton->SetParent(this->GetParameterFrame()->GetFrame());
  this->CenterResetButton->Create(this->Application, "");
  this->CenterResetButton->SetLabel("Set Plane Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("pack %s -side top -fill x -padx 2",
               this->CenterResetButton->GetWidgetName());

  // Normal -------------------------
  this->NormalEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->NormalEntry->SetPVSource(this);
  this->NormalEntry->Create(this->Application, "Normal:", 3, NULL,
                             "SetNormal", "GetNormal", NULL,
                             this->GetVTKSourceTclName());
  this->Widgets->AddItem(this->NormalEntry);
  this->Script("pack %s -side top -fill x",
               this->NormalEntry->GetWidgetName());

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

  // Offset -------------------------
  this->OffsetEntry->SetParent(this->GetParameterFrame()->GetFrame());
  this->OffsetEntry->SetPVSource(this);
  this->OffsetEntry->Create(this->Application, "Offset:", 1, NULL, "SetOffset",
                            "GetOffset", NULL, this->GetVTKSourceTclName());
  this->Widgets->AddItem(this->OffsetEntry);
  this->Script("pack %s -side top -fill x",
               this->OffsetEntry->GetWidgetName());

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

  this->CenterEntry->GetEntry(0)->SetValue(0.5*(bds[0]+bds[1]), 3);
  this->CenterEntry->GetEntry(1)->SetValue(0.5*(bds[2]+bds[3]), 3);
  this->CenterEntry->GetEntry(2)->SetValue(0.5*(bds[4]+bds[5]), 3);
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

  this->NormalEntry->GetEntry(0)->SetValue(-normal[0], 5);
  this->NormalEntry->GetEntry(1)->SetValue(-normal[1], 5);
  this->NormalEntry->GetEntry(2)->SetValue(-normal[2], 5);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalXCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(1.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(0.0, 1);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalYCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(1.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(0.0, 1);
}

//----------------------------------------------------------------------------
void vtkPVClipPlane::NormalZCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(1.0, 1);
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





