/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSphereWidget.cxx
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
#include "vtkPVSphereWidget.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkKWCompositeCollection.h"

int vtkPVSphereWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSphereWidget::vtkPVSphereWidget()
{
  this->CommandFunction = vtkPVSphereWidgetCommand;

  this->ResetButton = vtkKWPushButton::New();

  this->CenterEntry = vtkPVVectorEntry::New();
  this->CenterEntry->SetTraceReferenceObject(this);
  this->CenterEntry->SetTraceReferenceCommand("GetCenterEntry");

  this->RadiusEntry = vtkPVVectorEntry::New();
  this->RadiusEntry->SetTraceReferenceObject(this);
  this->RadiusEntry->SetTraceReferenceCommand("GetNormalEntry");

  this->SphereTclName = NULL;

  this->ObjectTclName = NULL;
  this->VariableName = NULL;
}

//----------------------------------------------------------------------------
vtkPVSphereWidget::~vtkPVSphereWidget()
{
  this->ResetButton->Delete();
  this->ResetButton = NULL;
  this->CenterEntry->Delete();
  this->CenterEntry = NULL;
  this->RadiusEntry->Delete();
  this->RadiusEntry = NULL;

  if (this->SphereTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", this->SphereTclName);
    this->SetSphereTclName(NULL);
    }

  this->SetObjectTclName(NULL);
  this->SetVariableName(NULL);
}

//----------------------------------------------------------------------------
vtkPVSphereWidget* vtkPVSphereWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVSphereWidget");
  if(ret)
    {
    return (vtkPVSphereWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVSphereWidget;
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::Create(vtkKWApplication *app)
{
  static int instanceCount = 0;
  char sphereTclName[256];
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
  
  // Create the implicit sphere associated with this widget.
  ++instanceCount;
  sprintf(sphereTclName, "pvSphere%d", instanceCount);
  pvApp->BroadcastScript("vtkSphere %s", sphereTclName);
  this->SetSphereTclName(sphereTclName);

  // create the top level
  this->Script("frame %s", this->GetWidgetName());
 
  this->CenterEntry->SetParent(this);
  this->CenterEntry->SetObjectVariable(sphereTclName, "Origin");
  this->CenterEntry->SetModifiedCommand(this->GetTclName(), 
                                        "ModifiedCallback");
  this->CenterEntry->Create(this->Application, "Center", 3, NULL, NULL);
  this->Script("pack %s -side top -fill x",
               this->CenterEntry->GetWidgetName());

  this->CenterResetButton->SetParent(this);
  this->CenterResetButton->Create(this->Application, "");
  this->CenterResetButton->SetLabel("Set Sphere Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("pack %s -side top -fill x -padx 2",
               this->CenterResetButton->GetWidgetName());

  // Normal -------------------------
  this->NormalEntry->SetParent(this);
  this->NormalEntry->SetObjectVariable(sphereTclName, "Normal");
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

  // Initialize the center of the sphere based on the input bounds.
  if (this->PVSource)
    {
    vtkPVData *input = this->PVSource->GetPVInput();
    if (input)
      {
      float bds[6];
      input->GetBounds(bds);
      pvApp->BroadcastScript("%s SetOrigin %f %f %f", sphereTclName,
                             0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                             0.5*(bds[4]+bds[5]));
      }
    }
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::CenterResetCallback()
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
void vtkPVSphereWidget::NormalCameraCallback()
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
  cam->GetViewSphereNormal(normal);

  this->NormalEntry->GetEntry(0)->SetValue(-normal[0], 5);
  this->NormalEntry->GetEntry(1)->SetValue(-normal[1], 5);
  this->NormalEntry->GetEntry(2)->SetValue(-normal[2], 5);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::NormalXCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(1.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(0.0, 1);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::NormalYCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(1.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(0.0, 1);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::NormalZCallback()
{
  this->NormalEntry->GetEntry(0)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(1)->SetValue(0.0, 1);
  this->NormalEntry->GetEntry(2)->SetValue(1.0, 1);

  this->NormalEntry->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Reset()
{
  this->CenterEntry->Reset();
  this->NormalEntry->Reset();

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if ( this->CenterEntry->GetModifiedFlag())
    {
    this->CenterEntry->Accept();
    }

  if ( this->NormalEntry->GetModifiedFlag())
    {
    this->NormalEntry->Accept();
    }

  // Set this here to keep this widget like others.
  if (this->ObjectTclName && this->VariableName && this->SphereTclName)
    {
    pvApp->BroadcastScript("%s Set%s %s", this->ObjectTclName,
                           this->VariableName, this->SphereTclName);
    }
  this->ModifiedFlag = 0;
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetObjectVariable(const char* objName, 
                                         const char* varName)
{
  this->SetObjectTclName(objName);
  this->SetVariableName(varName);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SaveInTclScript(ofstream *file)
{
  *file << "vtkSphere " << this->SphereTclName << endl;

  *file << "\t" << this->ObjectTclName << " Set" << this->VariableName
        << " " << this->SphereTclName << endl;

  this->CenterEntry->SaveInTclScript(file);
  this->NormalEntry->SaveInTclScript(file);
}




