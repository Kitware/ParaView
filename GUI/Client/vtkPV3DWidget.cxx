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

#include "vtk3DWidget.h"
#include "vtkCommand.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVRenderModule.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVRenderView.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkPVProcessModule.h"

#include "vtkKWEvent.h"
#include "vtkRM3DWidget.h"
//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPV3DWidget, "1.57");

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
  this->LabeledFrame = vtkKWLabeledFrame::New();
  this->Visibility   = vtkKWCheckButton::New();
  this->Frame        = vtkKWFrame::New();
  this->RM3DWidget = 0; //will be initialized by subclass
  this->ValueChanged = 1;
  this->ModifiedFlag = 1;
  this->Visible = 0;
  this->Placed = 0;
  this->UseLabel = 1;
  this->Widget3D = 0;
}

//----------------------------------------------------------------------------
vtkPV3DWidget::~vtkPV3DWidget()
{
  this->Observer->Delete();
  this->Visibility->Delete();
  this->LabeledFrame->Delete();
  this->Frame->Delete();
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
    this->LabeledFrame->SetLabel("3D Widget");
    
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

  vtkPVProcessModule *pm = pvApp->GetProcessModule();

  this->RM3DWidget->Create(pm,pm->GetRenderModule()->GetRendererID(),
    this->GetPVSource()->GetPVWindow()->GetInteractorID());


  this->ChildCreate(pvApp);

  if(this->RM3DWidget->GetWidget3DID().ID != 0)
    {
    this->Widget3D = 
      vtk3DWidget::SafeDownCast(pvApp->GetProcessModule()->
      GetObjectFromID(this->RM3DWidget->GetWidget3DID()));
    this->InitializeObservers(this->Widget3D);
    }
  this->PlaceWidget();
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::InitializeObservers(vtk3DWidget* widget3D) 
{
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetInteractor();
  if (iren)
    {
    //widget3D->SetInteractor(iren);
    widget3D->AddObserver(vtkCommand::InteractionEvent, 
                          this->Observer);
    widget3D->AddObserver(vtkCommand::PlaceWidgetEvent, 
                          this->Observer);
    widget3D->AddObserver(vtkCommand::StartInteractionEvent, 
                          this->Observer);
    widget3D->AddObserver(vtkCommand::EndInteractionEvent, 
                          this->Observer);
    widget3D->EnabledOff();
    }
  if(this->RM3DWidget)
    {
    this->RM3DWidget->AddObserver(vtkKWEvent::WidgetModifiedEvent,this->Observer);
    }
  this->Observer->Execute(widget3D, vtkCommand::InteractionEvent, 0);
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
void vtkPV3DWidget::AcceptInternal(vtkClientServerID id)
{
  this->PlaceWidget();
  this->Superclass::AcceptInternal(id);
  this->ModifiedFlag = 0;
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ResetInternal()
{
  this->Superclass::ResetInternal();
  if (this->AcceptCalled)
    {
    this->ModifiedFlag = 0;
    }
  this->ValueChanged = 0;
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
  this->Widget3D->SetCurrentRenderer(this->PVSource->GetPVWindow()->GetMainView()->GetRenderer());
  this->RM3DWidget->SetVisibility(visibility);
  this->AddTraceEntry("$kw(%s) SetVisibility %d", 
                      this->GetTclName(), visibility);
  this->Visibility->SetState(visibility);
  this->Visible = visibility;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Select()
{
  if ( this->Visible )
    {
    this->Widget3D->SetCurrentRenderer(this->PVSource->GetPVWindow()->GetMainView()->GetRenderer());
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
  this->RM3DWidget->SetVisibility(visibility);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetFrameLabel(const char* label)
{
  if ( this->LabeledFrame && this->UseLabel )
    {
    this->LabeledFrame->SetLabel(label);
    } 
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ActualPlaceWidget()
{
  double bds[6];
  this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);
  this->RM3DWidget->PlaceWidget(bds);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PlaceWidget()
{
  // We should really check to see if the input has changed (modified).

  if (!this->Placed && this->RM3DWidget->GetWidget3DID().ID != 0 )
    {
    this->ActualPlaceWidget();
    this->Placed = 1;
    this->ModifiedFlag = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ExecuteEvent(vtkObject*, unsigned long event, void*)
{
  if ( event == vtkCommand::PlaceWidgetEvent )
    {
    }
  else if ( event == vtkCommand::StartInteractionEvent )
    {
    this->PVSource->GetPVWindow()->InteractiveRenderEnabledOn();
    }
  else if ( event == vtkCommand::EndInteractionEvent )
    {
    this->PVSource->GetPVWindow()->InteractiveRenderEnabledOff();
    }
  else
    {
    this->ModifiedCallback();
    }
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
    this->GetPVApplication()->GetMainWindow()->GetInteractor()->Render();
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
  os << indent << "RM3DWidget: " << this->RM3DWidget << endl; 
}
