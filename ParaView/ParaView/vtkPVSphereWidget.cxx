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

#include "vtkCamera.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkSphereWidget.h"
#include "vtkRenderer.h"
#include "vtkPVProcessModule.h"

vtkStandardNewMacro(vtkPVSphereWidget);
vtkCxxRevisionMacro(vtkPVSphereWidget, "1.25.4.5");

int vtkPVSphereWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVSphereWidget::vtkPVSphereWidget()
{
  int cc;
  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->CenterEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->RadiusEntry = vtkKWEntry::New();
  this->CenterResetButton = vtkKWPushButton::New();
  this->SphereID.ID = 0;
  
  this->LastAcceptedCenter[0] = this->LastAcceptedCenter[1] =
    this->LastAcceptedCenter[2] = 0.0;
  this->LastAcceptedRadius = 1;
}

//----------------------------------------------------------------------------
vtkPVSphereWidget::~vtkPVSphereWidget()
{
  if (this->SphereID.ID)
    {
    vtkPVProcessModule* pv = this->GetPVApplication()->GetProcessModule();
    pv->DeleteStreamObject(this->SphereID);
    this->SphereID.ID = 0;
    pv->SendStreamToServer();
    }
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->CenterEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->RadiusEntry->Delete();
  this->CenterResetButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::CenterResetCallback()
{
  vtkPVSource *input;
  double bds[6];

  if (this->PVSource == NULL)
    {
    vtkErrorMacro("PVSource has not been set.");
    return;
    }

  input = this->PVSource->GetPVInput(0);
  if (input == NULL)
    {
    return;
    }
  input->GetDataInformation()->GetBounds(bds);
  this->SetCenter(0.5*(bds[0]+bds[1]),
                  0.5*(bds[2]+bds[3]),
                  0.5*(bds[4]+bds[5]));

  this->SetCenter();
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  this->SetCenter(this->LastAcceptedCenter);
  this->SetRadius(this->LastAcceptedRadius);
  
  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ActualPlaceWidget()
{
  float center[3];
  float radius;
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    center[cc] = atof(this->CenterEntry[cc]->GetValue());
    }
  radius = atof(this->RadiusEntry->GetValue());
 
  this->Superclass::ActualPlaceWidget();
  this->SetCenter(center[0], center[1], center[2]);
  this->SetRadius(radius);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::AcceptInternal(vtkClientServerID sourceID)  
{
  this->PlaceWidget();
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  if ( this->SphereID.ID )
    {
    vtkPVApplication *pvApp = static_cast<vtkPVApplication*>(
      this->Application); 
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    float val[3];
    int cc;
    for ( cc = 0; cc < 3; cc ++ )
      {
      val[cc] = atof( this->CenterEntry[cc]->GetValue() );
      }
    this->SetCenterInternal(val);
    float rad = atof(this->RadiusEntry->GetValue());
    this->SetRadiusInternal(rad);
    pm->GetStream() << vtkClientServerStream::Invoke << this->SphereID
                    << "SetCenter" << val[0] << val[1] << val[2] 
                    << vtkClientServerStream::End;
    pm->GetStream() << vtkClientServerStream::Invoke << this->SphereID
                    << "SetRadius" << rad
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    this->SetLastAcceptedCenter(val);
    this->SetLastAcceptedRadius(rad);
    }
  this->Superclass::AcceptInternal(sourceID);
}


//---------------------------------------------------------------------------
void vtkPVSphereWidget::Trace(ofstream *file)
{
  float rad;
  float val[3];
  int cc;
  
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof( this->CenterEntry[cc]->GetValue() );
    }
  *file << "$kw(" << this->GetTclName() << ") SetCenter "
        << val[0] << " " << val[1] << " " << val[2] << endl;

  rad = atof(this->RadiusEntry->GetValue());
  this->AddTraceEntry("$kw(%s) SetRadius %f", 
                      this->GetTclName(), rad);
  *file << "$kw(" << this->GetTclName() << ") SetRadius "
        << rad << endl;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::UpdateVTKObject(const char*)
{
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SaveInBatchScript(ofstream *file)
{
  *file << "vtkSphere " << "pvTemp" << this->SphereID << endl;
  *file << "\t" << this->SphereID << " SetCenter ";
  this->Script("%s GetCenter", this->SphereID);
  *file << this->Application->GetMainInterp()->result << endl;
  *file << "\t" << "pvTemp" << this->SphereID << " SetRadius ";
  this->Script("%s GetRadius", this->SphereID);
  *file << this->Application->GetMainInterp()->result << endl;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SphereID: " << this->SphereID << endl;
}

//----------------------------------------------------------------------------
vtkPVSphereWidget* vtkPVSphereWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVSphereWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetBalloonHelpString(const char *str)
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

    this->RadiusEntry->SetBalloonHelpString(this->BalloonHelpString);
    this->CenterResetButton->SetBalloonHelpString(this->BalloonHelpString);
    for (int i=0; i<3; i++)
      {
      this->CoordinateLabel[i]->SetBalloonHelpString(this->BalloonHelpString);
      this->CenterEntry[i]->SetBalloonHelpString(this->BalloonHelpString);
      }

    this->BalloonHelpInitialized = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ChildCreate(vtkPVApplication* pvApp)
{
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Sphere");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  this->Widget3DID = pm->NewStreamObject("vtkSphereWidget");
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "PlaceWidget" << 0 << 1 << 0 << 1 << 0 << 1 
                  << vtkClientServerStream::End;

  this->SphereID = pm->NewStreamObject("vtkSphere");
  
  this->SetFrameLabel("Sphere Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Center");
  this->Labels[1]->SetParent(this->Frame->GetFrame());
  this->Labels[1]->Create(pvApp, "");
  this->Labels[1]->SetLabel("Radius");

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
  this->RadiusEntry->SetParent(this->Frame->GetFrame());
  this->RadiusEntry->Create(pvApp, "");

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
  this->Script("grid %s %s - - -sticky ew",
               this->Labels[1]->GetWidgetName(),
               this->RadiusEntry->GetWidgetName());

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
    this->Script("bind %s <FocusOut> {%s SetCenter}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetCenter}",
                 this->CenterEntry[i]->GetWidgetName(),
                 this->GetTclName());
    }
  this->Script("bind %s <Key> {%s SetValueChanged}",
               this->RadiusEntry->GetWidgetName(),
               this->GetTclName());
  this->Script("bind %s <FocusOut> {%s SetRadius}",
               this->RadiusEntry->GetWidgetName(),
                 this->GetTclName());
  this->Script("bind %s <KeyPress-Return> {%s SetRadius}",
               this->RadiusEntry->GetWidgetName(),
               this->GetTclName());
  this->CenterResetButton->SetParent(this->Frame->GetFrame());
  this->CenterResetButton->Create(pvApp, "");
  this->CenterResetButton->SetLabel("Set Sphere Center to Center of Bounds");
  this->CenterResetButton->SetCommand(this, "CenterResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
               this->CenterResetButton->GetWidgetName());
  // Initialize the center of the sphere based on the input bounds.
  if (this->PVSource)
    {
    vtkPVSource *input = this->PVSource->GetPVInput(0);
    if (input)
      {
      double bds[6];
      input->GetDataInformation()->GetBounds(bds);
      pm->GetStream() << vtkClientServerStream::Invoke << this->SphereID 
                      << "SetCenter" 
                      << 0.5*(bds[0]+bds[1])
                      << 0.5*(bds[2]+bds[3])
                      << 0.5*(bds[4]+bds[5]) 
                      << vtkClientServerStream::End;
      pm->SendStreamToClientAndServer();
      this->SetCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                      0.5*(bds[4]+bds[5]));
      this->SetLastAcceptedCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                                  0.5*(bds[4]+bds[5]));
      pm->GetStream() << vtkClientServerStream::Invoke << this->SphereID 
                      << "SetRadius"  << 0.5*(bds[1]-bds[0])
                      << vtkClientServerStream::End;
      pm->SendStreamToClientAndServer();
      this->SetRadius(0.5*(bds[1]-bds[0]));
      this->SetLastAcceptedRadius(0.5*(bds[1]-bds[0]));
      }
    }

  this->SetBalloonHelpString(this->BalloonHelpString);

}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  vtkSphereWidget *widget = vtkSphereWidget::SafeDownCast(wdg);
  if ( widget )
    {
    float val[3];
    widget->GetCenter(val); 
    this->SetCenter(val[0], val[1], val[2]);
    float rad = widget->GetRadius();
    this->SetRadius(rad);
    }
  this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVSphereWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

void vtkPVSphereWidget::SetCenterInternal(float x, float y, float z)
{ 
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  this->CenterEntry[0]->SetValue(x);
  this->CenterEntry[1]->SetValue(y);
  this->CenterEntry[2]->SetValue(z);  
  if ( this->Widget3DID.ID )
    {
    pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                    << "SetCenter" << x << y << z
                    << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter(float x, float y, float z)
{
  this->SetCenterInternal(x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter()
{
  float val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    } 
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "SetCenter" << val[0] << val[1] << val[2]
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadiusInternal(float r)
{
  this->RadiusEntry->SetValue(r); 
  if ( this->Widget3DID.ID )
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    vtkPVProcessModule* pm = pvApp->GetProcessModule();
    pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
                  << "SetRadius" << r
                  << vtkClientServerStream::End;
    pm->SendStreamToClientAndServer();
    }
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius(float r)
{
  this->SetRadiusInternal(r);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  float val;
  val = atof(this->RadiusEntry->GetValue());
  pm->GetStream() << vtkClientServerStream::Invoke << this->SphereID
                  << "SetRadius" << val
                  << vtkClientServerStream::End;
  pm->SendStreamToClientAndServer();
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

