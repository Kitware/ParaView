/*=========================================================================

  Program:   ParaView
  Module:    vtkPVPointWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPVPointWidget.h"

#include "vtkCamera.h"
#include "vtkKWCompositeCollection.h"
#include "vtkKWEntry.h"
#include "vtkKWFrame.h"
#include "vtkKWLabel.h"
#include "vtkKWPushButton.h"
#include "vtkKWView.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkRenderer.h"
#include "vtkPVRenderModule.h"
#include "vtkPickPointWidget.h"

#include "vtkKWEvent.h"
#include "vtkPVRenderModule.h"
#include "vtkSMSourceProxy.h"
#include "vtkSMPointWidgetProxy.h"
#include "vtkSMDoubleVectorProperty.h"

vtkStandardNewMacro(vtkPVPointWidget);
vtkCxxRevisionMacro(vtkPVPointWidget, "1.40");

int vtkPVPointWidgetCommand(ClientData cd, Tcl_Interp *interp,
                        int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVPointWidget::vtkPVPointWidget()
{
  int cc;
  this->Labels[0] = vtkKWLabel::New();
  this->Labels[1] = vtkKWLabel::New();  
  for ( cc = 0; cc < 3; cc ++ )
    {
    this->PositionEntry[cc] = vtkKWEntry::New();
    this->CoordinateLabel[cc] = vtkKWLabel::New();
   }
  this->PositionResetButton = vtkKWPushButton::New();
  this->SetWidgetProxyXMLName("PointWidgetProxy");
}

//----------------------------------------------------------------------------
vtkPVPointWidget::~vtkPVPointWidget()
{
  int i;
  this->Labels[0]->Delete();
  this->Labels[1]->Delete();
  for (i=0; i<3; i++)
    {
    this->PositionEntry[i]->Delete();
    this->CoordinateLabel[i]->Delete();
    }
  this->PositionResetButton->Delete();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::PositionResetCallback()
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
  this->SetPosition(0.5*(bds[0]+bds[1]),
                  0.5*(bds[2]+bds[3]),
                  0.5*(bds[4]+bds[5]));
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::SetVisibility(int v)
{
  if (v)
    { // Get around the progress clearing the status text.
    // We can get rid of this when Andy adds the concept of a global status.
    this->Script("after 500 {%s SetStatusText {'p' picks a point.}}",
                 this->GetPVApplication()->GetMainWindow()->GetTclName());
    }
  else
    {
    this->GetPVApplication()->GetMainWindow()->SetStatusText("");
    }

  this->Superclass::SetVisibility(v);
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::Initialize()
{
  this->PlaceWidget();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ResetInternal()
{
  double pt[3];
  const char*variablename;
  
  vtkSMSourceProxy* sproxy = this->GetPVSource()->GetProxy();
  variablename = (this->VariableName)? this->VariableName : "Position";
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if (sdvp)
    {
    pt[0] = sdvp->GetElement(0);
    pt[1] = sdvp->GetElement(1);
    pt[2] = sdvp->GetElement(2);
    }
  else
    {
    vtkErrorMacro("Could not find property " << variablename 
      << " for widget: "<< sproxy->GetVTKClassName());
    return;
    }
  this->SetPositionInternal(pt[0],pt[1],pt[2]);
  this->Superclass::ResetInternal();
}

//---------------------------------------------------------------------------
void vtkPVPointWidget::Accept()
{
 
  int modFlag = this->GetModifiedFlag();
  double pt[3];
  const char* variablename;
  
  this->WidgetProxy->UpdateInformation();
  this->GetPositionInternal(pt);
  
  vtkSMSourceProxy* sproxy = this->GetPVSource()->GetProxy();
  variablename = (this->VariableName)? this->VariableName : "Position";
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));
  if(sdvp)
    {
    sdvp->SetElements3(pt[0], pt[1],pt[2]);
    }
  else
    {
    vtkErrorMacro("Could not find property "<<variablename<<" for widget: "<< sproxy->GetVTKClassName());
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

}

//---------------------------------------------------------------------------
void vtkPVPointWidget::Trace(ofstream *file)
{
  if ( ! this->InitializeTrace(file))
    {
    return;
    }

  *file << "$kw(" << this->GetTclName() << ") SetPosition "
        << this->PositionEntry[0]->GetValue() << " "
        << this->PositionEntry[1]->GetValue() << " "
        << this->PositionEntry[2]->GetValue() << endl;
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  vtkSMSourceProxy* sproxy = this->GetPVSource()->GetProxy();
  const char* variablename = (this->VariableName)? this->VariableName : "Position";
  vtkSMDoubleVectorProperty* sdvp = vtkSMDoubleVectorProperty::SafeDownCast(
    sproxy->GetProperty(variablename));

  this->WidgetProxy->SaveInBatchScript(file);
  // Point1
  if (sdvp)
    {  
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
          << variablename << "] SetElements3 "
          << sdvp->GetElement(0) << " "
          << sdvp->GetElement(1) << " "
          << sdvp->GetElement(2) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
      << variablename << "] SetControllerProxy $pvTemp"
      << this->WidgetProxy->GetID(0) << endl;
    *file << "  [$pvTemp" << sourceID << " GetProperty "
      << variablename << "] SetControllerProperty [$pvTemp"
      << this->WidgetProxy->GetID(0) 
      << " GetProperty Position]" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ChildCreate(vtkPVApplication* pvApp)
{
  int i;
  unsigned int ui;
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Point");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }
  // Widget needs the RenderModule for picking
  vtkProcessModule* pm = vtkProcessModule::GetProcessModule();
  for (ui=0; ui<this->WidgetProxy->GetNumberOfIDs(); ui++)
    {
    vtkPickPointWidget* widget = vtkPickPointWidget::SafeDownCast(
      pm->GetObjectFromID(this->WidgetProxy->GetID(ui)));
    if (widget)
      {
      widget->SetRenderModule(pm->GetRenderModule());
      }
    }
  
  this->SetFrameLabel("Point Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Position");

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
    this->PositionEntry[i]->SetParent(this->Frame->GetFrame());
    this->PositionEntry[i]->Create(pvApp, "");
    }

  this->Script("grid propagate %s 1",
               this->Frame->GetFrame()->GetWidgetName());

  this->Script("grid x %s %s %s -sticky ew",
               this->CoordinateLabel[0]->GetWidgetName(),
               this->CoordinateLabel[1]->GetWidgetName(),
               this->CoordinateLabel[2]->GetWidgetName());
  this->Script("grid %s %s %s %s -sticky ew",
               this->Labels[0]->GetWidgetName(),
               this->PositionEntry[0]->GetWidgetName(),
               this->PositionEntry[1]->GetWidgetName(),
               this->PositionEntry[2]->GetWidgetName());

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
                 this->PositionEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <FocusOut> {%s SetPosition}",
                 this->PositionEntry[i]->GetWidgetName(),
                 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetPosition}",
                 this->PositionEntry[i]->GetWidgetName(),
                 this->GetTclName());
    }
  this->PositionResetButton->SetParent(this->Frame->GetFrame());
  this->PositionResetButton->Create(pvApp, "");
  this->PositionResetButton->SetLabel("Set Point Position to Center of Bounds");
  this->PositionResetButton->SetCommand(this, "PositionResetCallback"); 
  this->Script("grid %s - - - - -sticky ew", 
               this->PositionResetButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ExecuteEvent(vtkObject* wdg, unsigned long l, void* p)
{
  if(l == vtkKWEvent::WidgetModifiedEvent)
    {
    double pos[3];
    this->WidgetProxy->UpdateInformation();
    this->GetPositionInternal(pos);
    this->PositionEntry[0]->SetValue(pos[0]);
    this->PositionEntry[1]->SetValue(pos[1]);
    this->PositionEntry[2]->SetValue(pos[2]);
    this->Render();
    }
 this->Superclass::ExecuteEvent(wdg, l, p);
}

//----------------------------------------------------------------------------
int vtkPVPointWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ActualPlaceWidget()
{
  this->Superclass::ActualPlaceWidget();

  double bounds[6];
  this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bounds);

  this->SetPosition((bounds[0]+bounds[1])/2,(bounds[2]+bounds[3])/2, 
                    (bounds[4]+bounds[5])/2);
  // Get around the progress clearing the status text.
  // We can get rid of this when Andy adds the concept of a global status.
  // fixme: Put the message in enable.
  this->Script("after 500 {%s SetStatusText {'p' picks a point.}}",
               this->GetPVApplication()->GetMainWindow()->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPositionInternal(double x, double y, double z)
{ 
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("Position")); 
  dvp->SetElements3(x,y,z);
  this->WidgetProxy->UpdateVTKObjects();

  this->PositionEntry[0]->SetValue(x);
  this->PositionEntry[1]->SetValue(y);
  this->PositionEntry[2]->SetValue(z);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPosition(double x, double y, double z)
{
  this->SetPositionInternal(x, y, z);
  this->AddTraceEntry("$kw(%s) SetPosition %f %f %f",
    this->GetTclName(), x, y, z);
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::GetPosition(double pt[3])
{
  if (pt == NULL || this->GetApplication() == NULL)
    {
    vtkErrorMacro("Cannot get your point.");
    return;
    }
  this->WidgetProxy->UpdateInformation();
  this->GetPositionInternal(pt);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::GetPositionInternal(double pt[3])
{
  vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
    this->WidgetProxy->GetProperty("PositionInfo"));
  pt[0] = dvp->GetElement(0);
  pt[1] = dvp->GetElement(1);
  pt[2] = dvp->GetElement(2);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPosition()
{
  if(!this->ValueChanged)
    {
    return;
    }
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->PositionEntry[cc]->GetValue());
    }
  this->SetPositionInternal(val[0], val[1], val[2]);
  this->Render();
  this->ValueChanged = 0;
}

 
