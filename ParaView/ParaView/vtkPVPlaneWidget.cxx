/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPlaneWidget.cxx
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
#include "vtkPVPlaneWidget.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkKWCompositeCollection.h"

int vtkPVPlaneWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPlaneWidget::vtkPVPlaneWidget()
{
  this->CommandFunction = vtkPVPlaneWidgetCommand;

  this->CenterEntry = vtkPVVectorEntry::New();
  this->CenterEntry->SetTraceReferenceObject(this);
  this->CenterEntry->SetTraceReferenceCommand("GetCenterEntry");
  this->CenterResetButton = vtkKWPushButton::New();

  this->NormalEntry = vtkPVVectorEntry::New();
  this->NormalEntry->SetTraceReferenceObject(this);
  this->NormalEntry->SetTraceReferenceCommand("GetNormalEntry");
  this->NormalButtonFrame = vtkKWWidget::New();
  this->NormalCameraButton = vtkKWPushButton::New();
  this->NormalXButton = vtkKWPushButton::New();
  this->NormalYButton = vtkKWPushButton::New();
  this->NormalZButton = vtkKWPushButton::New();

  this->PlaneTclName = NULL;
}

//----------------------------------------------------------------------------
vtkPVPlaneWidget::~vtkPVPlaneWidget()
{
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

  if (this->PlaneTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", this->PlaneTclName);
    this->SetPlaneTclName(NULL);
    }
}

//----------------------------------------------------------------------------
vtkPVPlaneWidget* vtkPVPlaneWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVPlaneWidget");
  if(ret)
    {
    return (vtkPVPlaneWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVPlaneWidget;
}


//----------------------------------------------------------------------------
void vtkPVPlaneWidget::Create(vtkKWApplication *app)
{
  static int instanceCount = 0;
  char planeTclName[256];
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(app);

  if (pvApp == NULL)
    {
    vtkErrorMacro("Expecting a PVApplication.");
    return;
    }

  if (this->Application != NULL)
    {
    vtkErrorMacro("Object has already been created.");
    return;
    }
  this->SetApplication(app);
  
  // Create the implicit plane associated with this widget.
  ++instanceCount;
  sprintf(planeTclName, "pvPlane%d", instanceCount);
  pvApp->BroadcastScript("vtkPlane %s", planeTclName);
  this->SetPlaneTclName(planeTclName);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());
 
  this->CenterEntry->SetParent(this);
  this->CenterEntry->SetObjectVariable(planeTclName, "Origin");
  this->CenterEntry->SetModifiedCommand(this->GetTclName(), 
                                        "ModifiedCallback");
  this->CenterEntry->Create(this->Application, "Center", 3, NULL, NULL);
  this->Script("pack %s -side top -fill x",
               this->CenterEntry->GetWidgetName());

  this->CenterResetButton->SetParent(this);
  this->CenterResetButton->Create(this->Application, "");
  this->CenterResetButton->SetLabel("Set Plane Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("pack %s -side top -fill x -padx 2",
               this->CenterResetButton->GetWidgetName());

  // Normal -------------------------
  this->NormalEntry->SetParent(this);
  this->NormalEntry->SetObjectVariable(planeTclName, "Normal");
  this->NormalEntry->SetModifiedCommand(this->GetTclName(), 
                                        "ModifiedCallback");
  this->NormalEntry->Create(this->Application, "Normal", 3, NULL, NULL);
  this->Script("pack %s -side top -fill x",
               this->NormalEntry->GetWidgetName());

  this->NormalButtonFrame->SetParent(this);
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

  // Initialize the center of the plane based on the input bounds.
  if (this->PVSource)
    {
    vtkPVData *input = this->PVSource->GetPVInput();
    if (input)
      {
      float bds[6];
      input->GetBounds(bds);
      this->Script("%s SetOrigin %f %f %f", planeTclName,
                   0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                   0.5*(bds[4]+bds[5]));
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVPlaneWidget::CenterResetCallback()
{
  vtkPVData *input;
  float bds[6];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  input = this->PVSource->GetPVInput();
  if (input == NULL)
    {
    return;
    }
  input->GetBounds(bds);

  this->CenterEntry->GetEntry(0)->SetValue(0.5*(bds[0]+bds[1]), 3);
  this->CenterEntry->GetEntry(1)->SetValue(0.5*(bds[2]+bds[3]), 3);
  this->CenterEntry->GetEntry(2)->SetValue(0.5*(bds[4]+bds[5]), 3);

  this->CenterEntry->ModifiedCallback();
}


//----------------------------------------------------------------------------
void vtkPVPlaneWidget::NormalCameraCallback()
{
  vtkKWView *view;
  vtkRenderer *ren;
  vtkCamera *cam;
  double normal[3];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  view = this->PVSource->GetView();
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

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::NormalXCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(1.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(0.0, 1);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::NormalYCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(1.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(0.0, 1);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::NormalZCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(1.0, 1);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::Reset()
{
  this->CenterEntry->Reset();
  this->NormalEntry->Reset();

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVPlaneWidget::Accept()
{
  if ( this->CenterEntry->GetModifiedFlag())
    {
    this->CenterEntry->Accept();
    }

  if ( this->NormalEntry->GetModifiedFlag())
    {
    this->NormalEntry->Accept();
    }

  this->ModifiedFlag = 0;
}






