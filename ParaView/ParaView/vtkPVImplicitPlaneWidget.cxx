/*=========================================================================

  Program:   ParaView
  Module:    vtkPVImplicitPlaneWidget.cxx
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
#include "vtkPVImplicitPlaneWidget.h"

#include "vtkCamera.h"
#include "vtkKWCompositeCollection.h"
#include "vtkPVProcessModule.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkMultiProcessController.h"
#include "vtkObjectFactory.h"
#include "vtkProperty.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkImplicitPlaneWidget.h"
#include "vtkRenderer.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVImplicitPlaneWidget);
vtkCxxRevisionMacro(vtkPVImplicitPlaneWidget, "1.10");

int vtkPVImplicitPlaneWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImplicitPlaneWidget::vtkPVImplicitPlaneWidget()
{
  int cc;

  vtkImplicitPlaneWidget *plane = vtkImplicitPlaneWidget::New();
  plane->SetPlaceFactor(1.0);
  plane->OutlineTranslationOff();
  this->Widget3D = plane;
  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->CenterEntry[cc] = vtkKWEntry::New();
    this->NormalEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->CenterResetButton = vtkKWPushButton::New();
  this->NormalButtonFrame = vtkKWWidget::New();
  this->NormalCameraButton = vtkKWPushButton::New();
  this->NormalXButton = vtkKWPushButton::New();
  this->NormalYButton = vtkKWPushButton::New();
  this->NormalZButton = vtkKWPushButton::New();
  this->PlaneTclName = 0;
}

//----------------------------------------------------------------------------
vtkPVImplicitPlaneWidget::~vtkPVImplicitPlaneWidget()
{
  if (this->PlaneTclName)
    {
    this->GetPVApplication()->BroadcastScript("%s Delete", 
                                              this->PlaneTclName);
    this->SetPlaneTclName(NULL);
    }
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->Delete();
    this->NormalEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->CenterResetButton->Delete();
  this->NormalButtonFrame->Delete();
  this->NormalCameraButton->Delete();
  this->NormalXButton->Delete();
  this->NormalYButton->Delete();
  this->NormalZButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::CenterResetCallback()
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
  input->GetDataInformation()->GetBounds(bds);
  this->CenterEntry[0]->SetValue(0.5*(bds[0]+bds[1]), 3);
  this->CenterEntry[1]->SetValue(0.5*(bds[2]+bds[3]), 3);
  this->CenterEntry[2]->SetValue(0.5*(bds[4]+bds[5]), 3);

  this->SetCenter();
}


//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalCameraCallback()
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

  this->SetNormal(-normal[0], -normal[1], -normal[2]);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalXCallback()
{
  this->SetNormal(1,0,0);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalYCallback()
{
  this->SetNormal(0,1,0);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::NormalZCallback()
{
  this->SetNormal(0,0,1);
  this->SetNormal();
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Reset(const char* sourceTclName)
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  static_cast<vtkImplicitPlaneWidget*>(this->Widget3D)->SetDrawPlane(0);
  if ( this->PlaneTclName )
    {
    this->Script("eval %s SetCenter [ %s GetOrigin ]", 
                 this->GetTclName(), this->PlaneTclName);
    this->Script("eval %s SetNormal [ %s GetNormal ]", 
                 this->GetTclName(), this->PlaneTclName);
    }
  this->Superclass::Reset(sourceTclName);
}
//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Reset()
{
  this->Reset(this->ObjectTclName);
}
//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ActualPlaceWidget()
{
  float center[3];
  float normal[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    center[cc] = atof(this->CenterEntry[cc]->GetValue());
    normal[cc] = atof(this->NormalEntry[cc]->GetValue());
    }
 
  this->Superclass::ActualPlaceWidget();
  this->SetCenter(center[0], center[1], center[2]);
  this->SetNormal(normal[0], normal[1], normal[2]);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Accept(const char* sourceTclName)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->PlaceWidget();
  static_cast<vtkImplicitPlaneWidget*>(this->Widget3D)->SetDrawPlane(0);

  // I guess this should be done in the initialization.
  if (this->VariableName && sourceTclName)
    {
    pvApp->BroadcastScript("%s Set%s %s", sourceTclName,
                           this->VariableName, this->PlaneTclName);                   
    }
  if ( this->PlaneTclName )
    {
    float val[3];
    int cc;
    for ( cc = 0; cc < 3; cc ++ )
      {
      val[cc] = atof( this->CenterEntry[cc]->GetValue() );
      }
    this->SetCenter(val);
    pvApp->BroadcastScript("%s SetOrigin %f %f %f", 
                           this->PlaneTclName,
                           val[0], val[1], val[2]);
    if (this->ModifiedFlag)
      {
      this->AddTraceEntry("$kw(%s) SetCenter %f %f %f", 
                          this->GetTclName(), val[0], val[1], val[2]);
      }
    for ( cc = 0; cc < 3; cc ++ )
      {
      val[cc] = atof( this->NormalEntry[cc]->GetValue() );
      }
    this->SetNormal(val);
    pvApp->BroadcastScript("%s SetNormal %f %f %f", 
                           this->PlaneTclName,
                           val[0], val[1], val[2]);
    if (this->ModifiedFlag)
      {
      this->AddTraceEntry("$kw(%s) SetNormal %f %f %f", 
                          this->GetTclName(), val[0], val[1], val[2]);
      }
    }

  this->Superclass::Accept(sourceTclName);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::Accept()
{
  this->Accept(this->ObjectTclName);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SaveInTclScript(ofstream *file)
{
  *file << "vtkPlane " << this->PlaneTclName << endl;
  *file << "\t" << this->PlaneTclName << " SetOrigin ";
  this->Script("%s GetOrigin", this->PlaneTclName);
  *file << this->Application->GetMainInterp()->result << endl;
  *file << "\t" << this->PlaneTclName << " SetNormal ";
  this->Script("%s GetNormal", this->PlaneTclName);
  *file << this->Application->GetMainInterp()->result << endl;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "PlaneTclName: " 
     << ( this->PlaneTclName ? this->PlaneTclName : "none" ) << endl;
}

vtkPVImplicitPlaneWidget* vtkPVImplicitPlaneWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVImplicitPlaneWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetBalloonHelpString(const char *str)
{

  // A little overkill.
  if (this->BalloonHelpString == NULL && str == NULL)
    {
    return;
    }

  // This check is needed to prevent errors when using
  // this->SetBalloonHelpString(this->BalloonHelpString)
  if (str != this->BalloonHelpString)
    {
    // Normal string stuff.
    if (this->BalloonHelpString)
      {
      delete [] this->BalloonHelpString;
      this->BalloonHelpString = NULL;
      }
    if (str != NULL)
      {
      this->BalloonHelpString = new char[strlen(str)+1];
      strcpy(this->BalloonHelpString, str);
      }
    }
  
  if ( this->Application && !this->BalloonHelpInitialized )
    {
    this->Labels[0]->SetBalloonHelpString(this->BalloonHelpString);
    this->Labels[1]->SetBalloonHelpString(this->BalloonHelpString);

    this->CenterResetButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalCameraButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalXButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalYButton->SetBalloonHelpString(this->BalloonHelpString);
    this->NormalZButton->SetBalloonHelpString(this->BalloonHelpString);

    for (int i=0; i<3; i++)
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->CenterEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->NormalEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ChildCreate(vtkPVApplication* pvApp)
{
  static int instanceCount = 0;
  char planeTclName[256];
  this->Widget3D->PlaceWidget(0, 1, 0, 1, 0, 1);
  ++instanceCount;
  sprintf(planeTclName, "pvPlane%d", instanceCount);

  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Plane");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  pvApp->BroadcastScript("vtkPlane %s", planeTclName);
  this->SetPlaneTclName(planeTclName);

  this->SetFrameLabel("Plane Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Center");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetLabel("Normal");

  int i;
  for (i=0; i<3; i++)
    {
    this->CoordinateLabel[i]->SetParent(this->Frame->GetFrame());
    this->CoordinateLabel[i]->Create(pvApp, "");
    char buffer[3];
    sprintf(buffer, "%c", "xyz"[i]);
    this->CoordinateLabel[i]->SetLabel(buffer);
    }
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->SetParent(this->Frame->GetFrame());
    this->CenterEntry[i]->Create(pvApp, "");
    }

  for (i=0; i<3; i++)    
    {
    this->NormalEntry[i]->SetParent(this->Frame->GetFrame());
    this->NormalEntry[i]->Create(pvApp, "");
    }

  this->Script("grid propagate %s 1",
               this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
               this->CoordinateLabel[0]->GetWidgetName(),
               this->CoordinateLabel[1]->GetWidgetName(),
               this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
               this->Labels[0]->GetWidgetName(),
               this->CenterEntry[0]->GetWidgetName(),
               this->CenterEntry[1]->GetWidgetName(),
               this->CenterEntry[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
               this->Labels[1]->GetWidgetName(),
               this->NormalEntry[0]->GetWidgetName(),
               this->NormalEntry[1]->GetWidgetName(),
               this->NormalEntry[2]->GetWidgetName());

  this->Script("grid columnconfigure %s 0 -weight 0", 
               this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 2", 
               this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 2", 
               this->Frame->GetFrame()->GetWidgetName());
  this->Script("grid columnconfigure %s 3 -weight 2", 
               this->Frame->GetFrame()->GetWidgetName());

  for (i=0; i<3; i++)
    {
    this->Script("bind %s <Key> {%s SetValueChanged}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <Key> {%s SetValueChanged}",
                 this->NormalEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetCenter}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetNormal}",
                 this->NormalEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetCenter}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetNormal}",
                 this->NormalEntry[i]->GetWidgetName(),
                 this->GetTclName());
    }
  this->CenterResetButton->SetParent(this->Frame->GetFrame());
  this->CenterResetButton->Create(pvApp, "");
  this->CenterResetButton->SetLabel("Set Plane Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
               this->CenterResetButton->GetWidgetName());

  this->NormalButtonFrame->SetParent(this->Frame->GetFrame());
  this->NormalButtonFrame->Create(pvApp, "frame", "");
  this->Script("grid %s - - - - -sticky ew", 
               this->NormalButtonFrame->GetWidgetName());

  this->NormalCameraButton->SetParent(this->NormalButtonFrame);
  this->NormalCameraButton->Create(pvApp, "");
  this->NormalCameraButton->SetLabel("Use Camera Normal");
  this->NormalCameraButton->SetCommand(this, "NormalCameraCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalCameraButton->GetWidgetName());
  this->NormalXButton->SetParent(this->NormalButtonFrame);
  this->NormalXButton->Create(pvApp, "");
  this->NormalXButton->SetLabel("X Normal");
  this->NormalXButton->SetCommand(this, "NormalXCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalXButton->GetWidgetName());
  this->NormalYButton->SetParent(this->NormalButtonFrame);
  this->NormalYButton->Create(pvApp, "");
  this->NormalYButton->SetLabel("Y Normal");
  this->NormalYButton->SetCommand(this, "NormalYCallback"); 
  this->Script("pack %s -side left -fill x -expand t",
               this->NormalYButton->GetWidgetName());
  this->NormalZButton->SetParent(this->NormalButtonFrame);
  this->NormalZButton->Create(pvApp, "");
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
      input->GetDataInformation()->GetBounds(bds);
      pvApp->BroadcastScript("%s SetOrigin %f %f %f", planeTclName,
                             0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                             0.5*(bds[4]+bds[5]));
      pvApp->BroadcastScript("%s SetNormal 0 0 1", planeTclName);
      this->Reset();
      }
    }

  vtkImplicitPlaneWidget* plane = 
    static_cast<vtkImplicitPlaneWidget*>(this->Widget3D);
  if (pvApp->GetProcessModule()->GetNumberOfPartitions() == 1)
    {
    plane->GetPlaneProperty()->SetOpacity(0.25);
    plane->GetSelectedPlaneProperty()->SetOpacity(0.25);
    }
  else
    {
    plane->GetPlaneProperty()->SetOpacity(1.0);
    plane->GetSelectedPlaneProperty()->SetOpacity(1.0);
    }

  this->SetBalloonHelpString(this->BalloonHelpString);

}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  vtkImplicitPlaneWidget *widget = vtkImplicitPlaneWidget::SafeDownCast(wdg);
  if ( widget )
    {
    float val[3];
    int cc;
    widget->GetOrigin(val); 
    for (cc=0; cc < 3; cc ++ )
      {
      this->CenterEntry[cc]->SetValue(val[cc], 5);
      }
    widget->GetNormal(val); 
    for (cc=0; cc < 3; cc ++ )
      {
      this->NormalEntry[cc]->SetValue(val[cc], 5);
      }
    if (!widget->GetDrawPlane())
      {
      widget->SetDrawPlane(1);
      }
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVImplicitPlaneWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenter(float x, float y, float z)
{
  this->CenterEntry[0]->SetValue(x, 3);
  this->CenterEntry[1]->SetValue(y, 3);
  this->CenterEntry[2]->SetValue(z, 3); 
  this->ModifiedFlag = 1;
  if ( this->Widget3D )
    {
    vtkImplicitPlaneWidget *plane = static_cast<vtkImplicitPlaneWidget*>(this->Widget3D);
    plane->SetOrigin(x, y, z); 
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormal(float x, float y, float z)
{
  this->NormalEntry[0]->SetValue(x, 3);
  this->NormalEntry[1]->SetValue(y, 3);
  this->NormalEntry[2]->SetValue(z, 3); 
  this->ModifiedFlag = 1;
  if ( this->Widget3D )
    {
    vtkImplicitPlaneWidget *plane = static_cast<vtkImplicitPlaneWidget*>(this->Widget3D);
    plane->SetNormal(x, y, z); 
    }
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetCenter()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    }
  this->SetCenter(val[0], val[1], val[2]);
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVImplicitPlaneWidget::SetNormal()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->NormalEntry[cc]->GetValue());
    }
  this->SetNormal(val[0], val[1], val[2]);
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}
