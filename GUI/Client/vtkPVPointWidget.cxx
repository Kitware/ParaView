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
#include "vtkPVData.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVDataInformation.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVVectorEntry.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkPickPointWidget.h"
#include "vtkRenderer.h"
#include "vtkPVRenderModuleProxyImplementation.h"

#include "vtkKWEvent.h"
#include "vtkRMPointWidget.h"
#include "vtkPVRenderModule.h"

vtkStandardNewMacro(vtkPVPointWidget);
vtkCxxRevisionMacro(vtkPVPointWidget, "1.33");

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
  this->RM3DWidget = static_cast<vtkRM3DWidget*>(vtkRMPointWidget::New());
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
  this->RM3DWidget->Delete();
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
void vtkPVPointWidget::ResetInternal()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }
  // Reset point
  static_cast<vtkRMPointWidget*>(this->RM3DWidget)->ResetInternal();
  this->Superclass::ResetInternal();
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::AcceptInternal(vtkClientServerID sourceID)  
{
  this->UpdateVTKObject();
  this->Superclass::AcceptInternal(sourceID);
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
void vtkPVPointWidget::UpdateVTKObject()  
{
  if ( this->VariableName && this->ObjectID.ID )
    {
    static_cast<vtkRMPointWidget*>(this->RM3DWidget)->
      UpdateVTKObject(this->ObjectID,this->VariableName);
    }
}


//----------------------------------------------------------------------------
void vtkPVPointWidget::SaveInBatchScript(ofstream *file)
{
  vtkClientServerID sourceID = this->PVSource->GetVTKSourceID(0);
  double pos[3];
  static_cast<vtkRMPointWidget*>(this->RM3DWidget)->GetPosition(pos);

  // Point1
  if (this->VariableName)
    {  
    *file << "  " << "[$pvTemp" << sourceID << " GetProperty " 
          << this->VariableName << "] SetElements3 "
          << pos[0] << " "
          << pos[1] << " "
          << pos[2] << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkPVPointWidget* vtkPVPointWidget::ClonePrototype(vtkPVSource* pvSource,
                                 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVPointWidget::SafeDownCast(clone);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::ChildCreate(vtkPVApplication* pvApp)
{
  if ((this->TraceNameState == vtkPVWidget::Uninitialized ||
       this->TraceNameState == vtkPVWidget::Default) )
    {
    this->SetTraceName("Point");
    this->SetTraceNameState(vtkPVWidget::SelfInitialized);
    }

  vtkPVProcessModule *pm = pvApp->GetProcessModule();

  // Widget needs the RenderModule for picking.
  vtkPickPointWidget *widget = vtkPickPointWidget::SafeDownCast(
             pm->GetObjectFromID(this->RM3DWidget->GetWidget3DID()));
  if (widget)
    {
    vtkPVRenderModuleProxyImplementation* rmp 
      = vtkPVRenderModuleProxyImplementation::New();
    rmp->SetPVRenderModule(pvApp->GetRenderModule());
    widget->SetRenderModuleProxy(rmp);
    rmp->Delete();
    }

  this->SetFrameLabel("Point Widget");
  this->Labels[0]->SetParent(this->Frame->GetFrame());
  this->Labels[0]->Create(pvApp, "");
  this->Labels[0]->SetLabel("Position");

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
      static_cast<vtkRMPointWidget*>(this->RM3DWidget)->GetPosition(pos);
      this->PositionEntry[0]->SetValue(pos[0]);
      this->PositionEntry[1]->SetValue(pos[1]);
      this->PositionEntry[2]->SetValue(pos[2]);
      this->Render();
    }
  else
    {
    vtkPickPointWidget *widget = vtkPickPointWidget::SafeDownCast(wdg);
    if ( !widget )
      {
      vtkErrorMacro( "This is not a point widget" );
      return;
      }
    double val[3];
    widget->GetPosition(val); 
    this->SetPosition(val[0], val[1], val[2]);
    this->Superclass::ExecuteEvent(wdg, l, p);
    }
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
  this->UpdateVTKObject();
  // Get around the progress clearing the status text.
  // We can get rid of this when Andy adds the concept of a global status.
  // fixme: Put the message in enable.
  this->Script("after 500 {%s SetStatusText {'p' picks a point.}}",
               this->GetPVApplication()->GetMainWindow()->GetTclName());
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPositionInternal(double x, double y, double z)
{ 
  ((vtkRMPointWidget*)this->RM3DWidget)->SetPosition(x,y,z);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPosition(double x, double y, double z)
{
  this->SetPositionInternal(x, y, z);
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
  ((vtkRMPointWidget*)this->RM3DWidget)->GetPosition(pt);
}

//----------------------------------------------------------------------------
void vtkPVPointWidget::SetPosition()
{
  double val[3];
  int cc;
  for ( cc = 0; cc < 3; cc ++ )
    {
    val[cc] = atof(this->PositionEntry[cc]->GetValue());
    }
  this->SetPositionInternal(val[0], val[1], val[2]);
  this->ModifiedCallback();
  this->ValueChanged = 0;
}

 
