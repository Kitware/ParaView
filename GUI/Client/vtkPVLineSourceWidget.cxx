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
#include "vtkPVLineWidget.h"
#include "vtkPVSource.h"
#include "vtkPVProcessModule.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMProxyManager.h"
#include "vtkSMSourceProxy.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkPVLineSourceWidget);
vtkCxxRevisionMacro(vtkPVLineSourceWidget, "1.23");

int vtkPVLineSourceWidgetCommand(ClientData cd, Tcl_Interp *interp,
                     int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::vtkPVLineSourceWidget()
{
  this->CommandFunction = vtkPVLineSourceWidgetCommand;
  this->LineWidget = vtkPVLineWidget::New();
  this->LineWidget->SetParent(this);
  this->LineWidget->SetTraceReferenceObject(this);
  this->LineWidget->SetTraceReferenceCommand("GetLineWidget");
  this->LineWidget->SetUseLabel(0);
}

//----------------------------------------------------------------------------
vtkPVLineSourceWidget::~vtkPVLineSourceWidget()
{
  this->LineWidget->Delete();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Create(vtkKWApplication *app)
{
  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(app, "frame", NULL))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

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

  this->LineWidget->SetPoint1VariableName("Point1");
  this->LineWidget->SetPoint2VariableName("Point2");
  this->LineWidget->SetResolutionVariableName("Resolution");
  this->LineWidget->SetPVSource(this->GetPVSource());
  this->LineWidget->SetModifiedCommand(this->GetPVSource()->GetTclName(), 
                                       "SetAcceptButtonColorToModified");
  
  this->LineWidget->Create(app);
  this->Script("pack %s -side top -fill both -expand true",
               this->LineWidget->GetWidgetName());
}


//----------------------------------------------------------------------------
int vtkPVLineSourceWidget::GetModifiedFlag()
{
  if (this->ModifiedFlag || this->LineWidget->GetModifiedFlag())
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::ResetInternal()
{
  if (this->AcceptCalled)
    {
    vtkSMDoubleVectorProperty *pt1p = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Point1"));
    vtkSMDoubleVectorProperty *pt2p = vtkSMDoubleVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Point2"));
    vtkSMIntVectorProperty *resp = vtkSMIntVectorProperty::SafeDownCast(
      this->SourceProxy->GetProperty("Resolution"));
    if (pt1p)
      {
      this->LineWidget->SetPoint1(pt1p->GetElement(0), pt1p->GetElement(1),
                                  pt1p->GetElement(2));
      }
    if (pt2p)
      {
      this->LineWidget->SetPoint2(pt2p->GetElement(0), pt2p->GetElement(1),
                                  pt2p->GetElement(2));
      }
    if (resp)
      {
      this->LineWidget->SetResolution(resp->GetElement(0));
      }
    this->ModifiedFlag = 0;
    }
  else
    {
    this->LineWidget->ActualPlaceWidget();
    }
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
    if (pt1p)
      {
      this->LineWidget->GetPoint1(pt);
      pt1p->SetElement(0, pt[0]);
      pt1p->SetElement(1, pt[1]);
      pt1p->SetElement(2, pt[2]);
      }
    if (pt2p)
      {
      this->LineWidget->GetPoint2(pt);
      pt2p->SetElement(0, pt[0]);
      pt2p->SetElement(1, pt[1]);
      pt2p->SetElement(2, pt[2]);
      }
    if (resp)
      {
      resp->SetElement(0, this->LineWidget->GetResolution());
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

//---------------------------------------------------------------------------
void vtkPVLineSourceWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  this->LineWidget->Trace(file);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Select()
{
  this->LineWidget->Select();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::Deselect()
{
  this->LineWidget->Deselect();
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::SaveInBatchScript(ofstream *file)
{
  double pt[3];

  if (!this->SourceProxy)
    {
    vtkErrorMacro("Source proxy must be set to save to a batch script.");
    return;
    }
  
  vtkClientServerID sourceID = this->SourceProxy->GetID(0);
  
  if (sourceID.ID == 0 || this->LineWidget == NULL)
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

  this->LineWidget->GetPoint1(pt);
  *file << "  [$pvTemp" << sourceID << " GetProperty Point1] "
        << "SetElements3 " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
  this->LineWidget->GetPoint2(pt);
  *file << "  [$pvTemp" << sourceID << " GetProperty Point2] "
        << "SetElements3 " << pt[0] << " " << pt[1] << " " << pt[2] << endl;
  *file << "  [$pvTemp" << sourceID << " GetProperty Resolution] "
        << "SetElements1 " << this->LineWidget->GetResolution() << endl;
  *file << "  $pvTemp" << sourceID << " UpdateVTKObjects" << endl;
  *file << endl;
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->LineWidget);
}

//----------------------------------------------------------------------------
void vtkPVLineSourceWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Line widget: " << this->LineWidget << endl;
}
