/*=========================================================================

  Program:   ParaView
  Module:    vtkPVSphereWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

#include "vtkKWEvent.h"
#include "vtkRMSphereWidget.h"

#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMProxy.h"

vtkStandardNewMacro(vtkPVSphereWidget);
vtkCxxRevisionMacro(vtkPVSphereWidget, "1.42");

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
  this->RM3DWidget = vtkRMSphereWidget::New();
  
  this->SphereProxy = 0;
  this->SphereProxyName = 0;
}

//----------------------------------------------------------------------------
vtkPVSphereWidget::~vtkPVSphereWidget()
{
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
  this->RM3DWidget->Delete();
  
  if (this->SphereProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("implicit_functions",
                                                    this->SphereProxyName);
    }
  this->SetSphereProxyName(0);
  if (this->SphereProxy)
    {
    this->SphereProxy->Delete();
    this->SphereProxy = 0;
    }
}

//----------------------------------------------------------------------------
vtkSMProxy* vtkPVSphereWidget::GetProxyByName(const char*)
{
  return this->SphereProxy;
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

}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->ResetInternal();
  this->Superclass::ResetInternal();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ActualPlaceWidget()
{
  double center[3];
  double radius;
  //int cc;
  //for ( cc = 0; cc < 3; cc ++ )
  //  {
  //  center[cc] = atof(this->CenterEntry[cc]->GetValue());
  //  }
  //radius = atof(this->RadiusEntry->GetValue());
  static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->GetCenter(center);
  radius = 
    static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->GetRadius();
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
  static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->UpdateVTKObject();

  vtkSMDoubleVectorProperty *cProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SphereProxy->GetProperty("Center"));
  vtkSMDoubleVectorProperty *rProp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SphereProxy->GetProperty("Radius"));
  if (cProp)
    {
    cProp->SetElement(0, this->CenterEntry[0]->GetValueAsFloat());
    cProp->SetElement(1, this->CenterEntry[1]->GetValueAsFloat());
    cProp->SetElement(2, this->CenterEntry[2]->GetValueAsFloat());
    }
  if (rProp)
    {
    rProp->SetElement(0, this->RadiusEntry->GetValueAsFloat());
    }
  this->SphereProxy->UpdateVTKObjects();
  
  this->Superclass::AcceptInternal(sourceID);
}


//---------------------------------------------------------------------------
void vtkPVSphereWidget::Trace(ofstream *file)
{
  double rad;
  double val[3];
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
  vtkClientServerID sphereID = this->SphereProxy->GetID(0);
  double center[3];
  static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->
    GetLastAcceptedCenter(center);

  double radius = static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->
    GetLastAcceptedRadius();

  *file << endl;
  *file << "set pvTemp" << sphereID.ID
        << " [$proxyManager NewProxy implicit_functions Sphere]"
        << endl;
  *file << "  $proxyManager RegisterProxy implicit_functions pvTemp"
        << sphereID.ID << " $pvTemp" << sphereID.ID
        << endl;
  *file << "  $pvTemp" << sphereID.ID << " UnRegister {}" << endl;
  *file << "  [$pvTemp" << sphereID.ID << " GetProperty Center] "
        << "SetElements3 " 
        << center[0] << " "
        << center[1] << " "
        << center[2] << " "
        << endl;
  *file << "  [$pvTemp" << sphereID.ID << " GetProperty Radius] "
        << "SetElements1 "
        << radius << endl << endl;
  *file << "  $pvTemp" << sphereID.ID << " UpdateVTKObjects" << endl;
  *file << endl;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
//  os << indent << "SphereID: " << this->SphereID << endl;
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
  
  if ( this->GetApplication() && !this->BalloonHelpInitialized )
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
      this->SetCenter(0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                      0.5*(bds[4]+bds[5]));
      static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->SetLastAcceptedCenter(
                      0.5*(bds[0]+bds[1]), 0.5*(bds[2]+bds[3]),
                                  0.5*(bds[4]+bds[5]));
      this->SetRadius(0.5*(bds[1]-bds[0]));
      static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->SetLastAcceptedRadius(
                      0.5*(bds[1]-bds[0]));
      }
    }

  this->SetBalloonHelpString(this->BalloonHelpString);

  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->SphereProxy = pm->NewProxy("implicit_functions", "Sphere");
  ostrstream str;
  static int instanceCount = 0;
  str << "Sphere" << instanceCount << ends;
  instanceCount++;
  this->SetSphereProxyName(str.str());
  pm->RegisterProxy("implicit_functions", this->SphereProxyName,
                    this->SphereProxy);
  this->SphereProxy->CreateVTKObjects(1);
  str.rdbuf()->freeze(0);
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  if(l == vtkKWEvent::WidgetModifiedEvent && wdg == this->RM3DWidget)
    {
    double center[3];
    double radius;
    static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->GetCenter(center);
    radius = static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->GetRadius();
    this->CenterEntry[0]->SetValue(center[0]);
    this->CenterEntry[1]->SetValue(center[1]);
    this->CenterEntry[2]->SetValue(center[2]);
    this->RadiusEntry->SetValue(radius);
    this->Render();
    this->ModifiedCallback();
    this->ValueChanged = 0;
    }
  else
    {
    vtkSphereWidget *widget = vtkSphereWidget::SafeDownCast(wdg);
    if ( widget )
      {
      double val[3];
      widget->GetCenter(val); 
      this->SetCenter(val[0], val[1], val[2]);
      double rad = widget->GetRadius();
      this->SetRadius(rad);
      }
    this->Superclass::ExecuteEvent(wdg, l, p);
    }
}

//----------------------------------------------------------------------------
int vtkPVSphereWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

void vtkPVSphereWidget::SetCenterInternal(double x, double y, double z)
{
  static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->SetCenter(x,y,z);
  //vtkPVApplication *pvApp = this->GetPVApplication();
  //vtkPVProcessModule* pm = pvApp->GetProcessModule();
  //this->CenterEntry[0]->SetValue(x);
  //this->CenterEntry[1]->SetValue(y);
  //this->CenterEntry[2]->SetValue(z);  
  //if ( this->Widget3DID.ID )
  //  {
  //  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
  //                  << "SetCenter" << x << y << z
  //                  << vtkClientServerStream::End;
  //  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  //  }
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter(double x, double y, double z)
{
  this->SetCenterInternal(x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetCenter()
{
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->CenterEntry[cc]->GetValue());
    } 
  this->SetCenterInternal(val[0],val[1],val[2]);
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadiusInternal(double r)
{
  static_cast<vtkRMSphereWidget*>(this->RM3DWidget)->SetRadius(r);
  //this->RadiusEntry->SetValue(r); 
  //if ( this->Widget3DID.ID )
  //  {
  //  vtkPVApplication *pvApp = this->GetPVApplication();
  //  vtkPVProcessModule* pm = pvApp->GetProcessModule();
  //  pm->GetStream() << vtkClientServerStream::Invoke << this->Widget3DID
  //                << "SetRadius" << r
  //                << vtkClientServerStream::End;
  //  pm->SendStream(vtkProcessModule::CLIENT|vtkProcessModule::RENDER_SERVER);
  //  }
}


//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius(double r)
{
  this->SetRadiusInternal(r);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::SetRadius()
{
  double val;
  val = atof(this->RadiusEntry->GetValue());
  this->SetRadiusInternal(val);
  this->Render();
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPVSphereWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->RadiusEntry);
  this->PropagateEnableState(this->CenterResetButton);

  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PropagateEnableState(this->CenterEntry[cc]);
    this->PropagateEnableState(this->CoordinateLabel[cc]);
    }

  this->PropagateEnableState(this->Labels[0]);
  this->PropagateEnableState(this->Labels[1]);
}


