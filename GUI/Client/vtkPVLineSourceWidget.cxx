/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineSourceWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVLineSourceWidget.h"

#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVSource.h"
#include "vtkPVProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"
#include "vtkSM3DWidgetProxy.h"
//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLineSourceWidget);
vtkCxxRevisionMacro(vtkPVLineSourceWidget, "1.24");

int vtkPVLineSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::vtkPVLineSourceWidget()
{
  this->CommandFunction = vtkPVLineSourceWidgetCommand;
  this->SourceProxyName = 0;
  this->SourceProxy = 0;
}

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::~vtkPVLineSourceWidget()
{
  if(this->SourceProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("source",
      this->SourceProxyName);
    }
  this->SetSourceProxyName(0);
  if(this->SourceProxy)
    {
    this->SourceProxy->Delete();
    this->SourceProxy = 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags
  this->Superclass::Create(app);
  
  static int proxyNum = 0;
  vtkSMProxyManager *pm = vtkSMObject::GetProxyManager();
  this->SourceProxy = vtkSMSourceProxy::SafeDownCast(
    pm->NewProxy("sources", "LineSource"));
  ostrstream str;
  str << "LineSource" << proxyNum << ends;
  this->SetSourceProxyName(str.str());
  pm->RegisterProxy("sources", this->SourceProxyName, this->SourceProxy);
  proxyNum++;
  str.rdbuf()->freeze(0);
  this->SourceProxy->CreateVTKObjects(1);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::ResetInternal()
{
  if (!this->AcceptCalled)
    {
    this->ActualPlaceWidget();
    return;
    }
  vtkSMDoubleVectorProperty *pt1p = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point1"));
  vtkSMDoubleVectorProperty *pt2p = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point2"));
  vtkSMIntVectorProperty *resp = vtkSMIntVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Resolution"));
  if (pt1p)
    {
    this->SetPoint1Internal(pt1p->GetElement(0), pt1p->GetElement(1),
      pt1p->GetElement(2));
    }
  if (pt2p)
    {
    this->SetPoint2Internal(pt2p->GetElement(0), pt2p->GetElement(1),
      pt2p->GetElement(2));
    }
  if (resp)
    {
    this->SetResolution(resp->GetElement(0));
    }
  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Accept()
{
  int modFlag = this->GetModifiedFlag();

  if (modFlag)
    {
    vtkSMDoubleVectorProperty *pt1p = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Point1"));
    vtkSMDoubleVectorProperty *pt2p = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Point2"));
    vtkSMIntVectorProperty *resp = vtkSMIntVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Resolution"));
    double pt[3];
    this->WidgetProxy->UpdateInformation();
    if (pt1p)
      {
      this->GetPoint1Internal(pt);
      pt1p->SetElement(0, pt[0]);
      pt1p->SetElement(1, pt[1]);
      pt1p->SetElement(2, pt[2]);
      }
    if (pt2p)
      {
      this->GetPoint2Internal(pt);
      pt2p->SetElement(0, pt[0]);
      pt2p->SetElement(1, pt[1]);
      pt2p->SetElement(2, pt[2]);
      }
    if (resp)
      {
      resp->SetElement(0, this->GetResolutionInternal());
      }
    this->SourceProxy->UpdateVTKObjects();
    this->SourceProxy->UpdatePipeline();
    }
  this->ModifiedFlag = 0;

  // I put this after the accept internal, because
  // vtkPVGroupWidget inactivates and builds an input list ...
  // Putting this here simplifies subclasses AcceptInternal methods.
  if (modFlag)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    ofstream* file = pvApp->GetTraceFile();
    if (file)
      {
      this->Trace(file);
      }
    }

  this->AcceptCalled = 1;
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::SaveInBatchScript(ofstream *file)
{
  if (!this->SourceProxy)
  {
  vtkErrorMacro("Source proxy must be set to save to a batch script.");
  return;
  }

  vtkClientServerID sourceID = this->SourceProxy->GetID(0);

  if (sourceID.ID == 0)
  {
  vtkErrorMacro("Sanity check failed. " << this->GetClassName());
  return;
  } 

  *file << endl;
  *file << "set pvTemp" << sourceID
    << " [$proxyManager NewProxy sources LineSource]" << endl;
  *file << "  $proxyManager RegisterProxy sources pvTemp"
    << sourceID << " $pvTemp" << sourceID << endl;
  *file << "  $pvTemp" << sourceID << " UnRegister {}" << endl;

  
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point1"));
  if(dvp)
    {
  *file << "  [$pvTemp" << sourceID << " GetProperty Point1] "
    << "SetElements3 " 
    << dvp->GetElement(0) << " " 
    << dvp->GetElement(1) << " " 
    << dvp->GetElement(2) << endl;
    }
  
  dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Point2"));
  if (dvp)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty Point2] "
      << "SetElements3 " 
      << dvp->GetElement(0) << " " 
      << dvp->GetElement(1) << " " 
      << dvp->GetElement(2) << endl;
    }
  vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
    this->SourceProxy->GetProperty("Resolution"));
  if(ivp)
    {
    *file << "  [$pvTemp" << sourceID << " GetProperty Resolution] "
      << "SetElements1 " << ivp->GetElement(0) << endl;
    }
  *file << "  $pvTemp" << sourceID << " UpdateVTKObjects" << endl;
  *file << endl;

  this->WidgetProxy->SaveInBatchScript(file);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SourceProxyName: " << 
    (this->SourceProxyName? this->SourceProxyName: "None") << endl;
  os << indent << "SourceProxy: " << this->SourceProxy << endl;
}
