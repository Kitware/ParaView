/*=========================================================================

  Program:   ParaView
  Module:    vtkPV3DWidget.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPV3DWidget.h"
#include "vtkPVConfig.h"
#include "vtkCommand.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWFrameLabeled.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkSMRenderModuleProxy.h"
#include "vtkPVDisplayGUI.h"
#include "vtkPVDataInformation.h"

#include "vtkPVSource.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"

#include "vtkSMProxyManager.h"
#include "vtkSM3DWidgetProxy.h"
#include "vtkSMIntVectorProperty.h"
#include "vtkSMDoubleVectorProperty.h"
#include "vtkSMProxyProperty.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPV3DWidget, "1.67.2.5");

//===========================================================================
//***************************************************************************
class vtkPV3DWidgetObserver : public vtkCommand
{
public:
  static vtkPV3DWidgetObserver *New() 
    {return new vtkPV3DWidgetObserver;};

  vtkPV3DWidgetObserver()
    {
      this->PV3DWidget = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long event,  
                       void* calldata)
    {
      if ( this->PV3DWidget )
        {
        this->PV3DWidget->ExecuteEvent(wdg, event, calldata);
        }
    }

  vtkPV3DWidget* PV3DWidget;
};
//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPV3DWidget::vtkPV3DWidget()
{
  this->Observer     = vtkPV3DWidgetObserver::New();
  this->Observer->PV3DWidget = this;
  this->LabeledFrame = vtkKWFrameLabeled::New();
  this->Visibility   = vtkKWCheckButton::New();
  this->Frame        = vtkKWFrame::New();
  this->ValueChanged = 1;
  this->ModifiedFlag = 1;
  this->Visible = 0;
  this->Placed = 0;
  this->UseLabel = 1;
  this->WidgetProxy = 0;
  this->WidgetProxyName = 0;
  this->WidgetProxyXMLName = 0;
}

//----------------------------------------------------------------------------
vtkPV3DWidget::~vtkPV3DWidget()
{
  this->Observer->Delete();
  this->Visibility->Delete();
  this->LabeledFrame->Delete();
  this->Frame->Delete();
  if (this->WidgetProxyName)
    {
    vtkSMObject::GetProxyManager()->UnRegisterProxy("3d_widgets",this->WidgetProxyName);
    }
  this->SetWidgetProxyName(0);
  if (this->WidgetProxy)
    {
    
    vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
    if (rm)
      {
      vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
        rm->GetProperty("Displays"));
      if (pp)
        {
        pp->RemoveProxy(this->WidgetProxy);
        rm->UpdateVTKObjects();
        }
      }
    this->WidgetProxy->Delete();
    this->WidgetProxy = 0;
    }
  this->SetWidgetProxyXMLName(0);
}


//----------------------------------------------------------------------------
void vtkPV3DWidget::Create(vtkKWApplication *app)
{
  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(app);

  // Call the superclass to create the widget and set the appropriate flags

  if (!this->vtkKWWidget::Create(pvApp, "frame", "-bd 0 -relief flat"))
    {
    vtkErrorMacro("Failed creating widget " << this->GetClassName());
    return;
    }

  vtkKWWidget* parent = this;

  if (this->UseLabel)
    {
    this->LabeledFrame->SetParent(this);
    this->LabeledFrame->Create(pvApp, 0);
    this->LabeledFrame->SetLabelText("3D Widget");
    
    this->Script("pack %s -fill both -expand 1", 
                 this->LabeledFrame->GetWidgetName());

    parent = this->LabeledFrame->GetFrame();
    }

  this->Frame->SetParent(parent);
  this->Frame->Create(pvApp, 0);
  this->Script("pack %s -fill both -expand 1", 
               this->Frame->GetWidgetName());
  
  this->Visible = pvApp->GetDisplay3DWidgets();
  this->Visibility->SetParent(parent);
  this->Visibility->Create(pvApp, "");
  this->Visibility->SetText("Visibility");
  this->Visibility->SetBalloonHelpString(
    "Toggle the visibility of the 3D widget on/off.");
  if ( this->Visible )
    {
    this->Visibility->SetState(1);
    }
  this->Visibility->SetCommand(this, "SetVisibility");
    
  this->Script("pack %s -fill x -expand 1",
               this->Visibility->GetWidgetName());

  //Create the WidgetProxy
  vtkSMProxyManager *pxm = vtkSMObject::GetProxyManager();
  static int proxyNum = 0;
  
  if (!this->WidgetProxyXMLName)
    {
    vtkErrorMacro("ProxyXMLName not set. Cannot determine what proxy to create");
    return;
    }
  this->WidgetProxy = vtkSM3DWidgetProxy::SafeDownCast(pxm->NewProxy("3d_widgets",this->WidgetProxyXMLName));
  if(!this->WidgetProxy)
    {
    vtkErrorMacro("Failed to create proxy " << this->WidgetProxyXMLName);
    return;
    }
  ostrstream str;
  str << "PV3DWidget_" << this->WidgetProxyXMLName << proxyNum << ends;
  this->SetWidgetProxyName(str.str());
  pxm->RegisterProxy("3d_widgets",this->WidgetProxyName, this->WidgetProxy);
  proxyNum++;
  str.rdbuf()->freeze(0);
  this->WidgetProxy->SetServers(
    vtkProcessModule::CLIENT | vtkProcessModule::RENDER_SERVER);
  this->WidgetProxy->CreateVTKObjects(1);

  // Add to the render module.
  vtkSMRenderModuleProxy* rm = this->GetPVApplication()->GetRenderModuleProxy();
  vtkSMProxyProperty* pp = vtkSMProxyProperty::SafeDownCast(
    rm->GetProperty("Displays"));
  if (!pp)
    {
    vtkErrorMacro("Failed to find property Displays on RenderModuleProxy.");
    }
  else
    {
    pp->AddProxy(this->WidgetProxy);
    rm->UpdateVTKObjects();
    }

  this->InitializeObservers(this->WidgetProxy);
  this->ChildCreate(pvApp);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::InitializeObservers(vtkSM3DWidgetProxy* widgetproxy)
{
  if(widgetproxy)
    {
    // TODO: verify if we need the Start and End Interaction events at all.
    widgetproxy->AddObserver(vtkCommand::WidgetModifiedEvent,this->Observer);
    widgetproxy->AddObserver(vtkCommand::StartInteractionEvent,this->Observer);
    widgetproxy->AddObserver(vtkCommand::EndInteractionEvent,this->Observer);
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::CopyProperties(vtkPVWidget* clone, 
                                   vtkPVSource* pvSource,
                                   vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPV3DWidget* pvlw = vtkPV3DWidget::SafeDownCast(clone);
  if (pvlw)
    {
    pvlw->SetUseLabel(this->GetUseLabel());
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLineWidget.");
    }
}


//----------------------------------------------------------------------------
void vtkPV3DWidget::Accept()
{
  this->ModifiedFlag = 0;
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Initialize()
{
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ResetInternal()
{
  this->Superclass::ResetInternal();
  this->ValueChanged = 0;
  this->Render();
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetValueChanged()
{
  this->ValueChanged = 1;
  this->ModifiedCallback();
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetVisibility()
{
  int visibility = this->Visibility->GetState();
  this->SetVisibility(visibility);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetVisibility(int visibility)
{
  if ( visibility )
    {
    this->PlaceWidget();
    }
  this->SetVisibilityNoTrace(visibility);
  this->AddTraceEntry("$kw(%s) SetVisibility %d", this->GetTclName(), visibility);
  this->Visibility->SetState(visibility);
  this->Visible = visibility;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Select()
{
  if (this->Visible && this->WidgetProxy)
    {
    this->SetVisibilityNoTrace(1);
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Deselect()
{
  this->SetVisibilityNoTrace(0);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetVisibilityNoTrace(int visibility)
{  
  if(this->WidgetProxy)
    {
    vtkSMIntVectorProperty* ivp = vtkSMIntVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("Visibility"));
    ivp->SetElements1(visibility);
    this->WidgetProxy->UpdateVTKObjects();
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetFrameLabel(const char* label)
{
  if ( this->LabeledFrame && this->UseLabel )
    {
    this->LabeledFrame->SetLabelText(label);
    } 
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ActualPlaceWidget()
{
  double bds[6];
  if (  this->PVSource->GetPVInput(0))
    {
    this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);
    }
  else
    {
    bds[0] = bds[2] = bds[4] = 0.0;
    bds[1] = bds[3] = bds[5] = 1.0;
    }
  this->PlaceWidget(bds);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PlaceWidget(double bds[6])
{
  if(this->WidgetProxy)
    {
    vtkSMDoubleVectorProperty* dvp = vtkSMDoubleVectorProperty::SafeDownCast(
      this->WidgetProxy->GetProperty("PlaceWidget"));
    if(dvp)
      {
      dvp->SetElements(bds);
      }
    this->WidgetProxy->UpdateVTKObjects();
    }
}
//----------------------------------------------------------------------------
void vtkPV3DWidget::PlaceWidget()
{
  // We should really check to see if the input has changed (modified).
  if (!this->Placed)
    {
    this->ActualPlaceWidget();
    this->Placed = 1;
    this->ModifiedFlag = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ExecuteEvent(vtkObject* obj, unsigned long event, void*calldata)
{
  if (vtkSM3DWidgetProxy::SafeDownCast(obj))
    {
    if (event == vtkCommand::WidgetModifiedEvent)
      {
      this->ModifiedCallback();
      }
    }
  this->Superclass::ExecuteEvent(obj, event, calldata);
}

//----------------------------------------------------------------------------
int vtkPV3DWidget::ReadXMLAttributes(vtkPVXMLElement* element,
  vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  if(!element->GetScalarAttribute("use_label", &this->UseLabel))
    {
    this->UseLabel = 1;
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Render()
{
  if ( this->Placed )
    {
    this->GetPVApplication()->GetRenderModuleProxy()->StillRender();
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::UpdateEnableState()
{
  this->Superclass::UpdateEnableState();

  this->PropagateEnableState(this->Frame);
  this->PropagateEnableState(this->LabeledFrame);
  this->PropagateEnableState(this->Visibility);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Use Label: " << (this->UseLabel?"on":"off") << endl;
  os << indent << "3D Widget:" << endl;
  os << indent << "WidgetProxyName: " << (this->WidgetProxyName? this->WidgetProxyName : "NULL") << endl;
  os << indent << "WidgetProxyXMLName: " << (this->WidgetProxyXMLName? 
    this->WidgetProxyXMLName : "NULL") << endl;
  os << indent << "WidgetProxy: " << this->WidgetProxy <<endl;
  
}
