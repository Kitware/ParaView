/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPV3DWidget.cxx
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
#include "vtkPV3DWidget.h"

#include "vtk3DWidget.h"
#include "vtkCommand.h"
#include "vtkKWCheckButton.h"
#include "vtkKWFrame.h"
#include "vtkKWLabeledFrame.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVDataInformation.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVPart.h"
#include "vtkPVProcessModule.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"

//----------------------------------------------------------------------------
vtkCxxRevisionMacro(vtkPV3DWidget, "1.39.2.2");

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
  this->ValueChanged = 1;
  this->ModifiedFlag = 1;
  this->Widget3DTclName = 0;
  this->Visible = 0;
  this->Placed = 0;
  this->UseLabel = 1;
  this->Widget3D = 0;
}

//----------------------------------------------------------------------------
vtkPV3DWidget::~vtkPV3DWidget()
{
  if (this->Widget3DTclName)
    {
    vtkPVApplication *pvApp = this->GetPVApplication();
    pvApp->GetProcessModule()->ServerScript(
      "%s EnabledOff", this->Widget3DTclName);
    pvApp->GetProcessModule()->ServerScript(
      "%s Delete", this->Widget3DTclName);
    this->SetWidget3DTclName(NULL);
    }
  this->Observer->Delete();
  this->Visibility->Delete();
  this->LabeledFrame->Delete();
  this->Frame->Delete();
}


//----------------------------------------------------------------------------
void vtkPV3DWidget::Create(vtkKWApplication *kwApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("3D Widget already created");
    return;
    }

  vtkPVApplication* pvApp = vtkPVApplication::SafeDownCast(kwApp);

  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

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

  this->ChildCreate(pvApp);

  this->Widget3D = vtk3DWidget::SafeDownCast(pvApp->TclToVTKObject(this->Widget3DTclName));

  // Only initialize observers on the UI process.
  if (this->Widget3DTclName)
    {
    // Default/dummy interactor for satelite procs.
    pvApp->GetProcessModule()->ServerScript(
      "%s SetInteractor pvRenderWindowInteractor", this->Widget3DTclName);
    this->Script("%s InitializeObservers %s", this->GetTclName(),
                 this->Widget3DTclName);
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
void vtkPV3DWidget::AcceptInternal(const char*)
{
  this->PlaceWidget();
  this->ModifiedFlag = 0;
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ResetInternal()
{
  this->ModifiedFlag = 0;
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
  vtkPVApplication *pvApp = this->GetPVApplication();
  if ( visibility )
    {
    this->PlaceWidget();
    }

  pvApp->GetProcessModule()->ServerScript(
    "%s SetEnabled %d", this->Widget3DTclName, visibility);
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
  this->GetPVApplication()->GetProcessModule()->ServerScript(
    "%s SetEnabled %d", this->Widget3DTclName, visibility);
  //is->Widget3D->SetEnabled(visibility);  
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
  // I do not think we actually need an input.

  //vtkDataSet* data = 0;
  //if ( this->PVSource->GetPVInput() )
  //  {
  //  data = this->PVSource->GetPVInput()->GetPVPart()->GetVTKData();
  //  }
  //this->Widget3D->SetInput(data);
  float bds[6];
  this->PVSource->GetPVInput(0)->GetDataInformation()->GetBounds(bds);
  vtkPVApplication *pvApp = this->GetPVApplication();
  pvApp->GetProcessModule()->ServerScript(
    "%s PlaceWidget %f %f %f %f %f %f", this->Widget3DTclName, 
    bds[0], bds[1], bds[2], bds[3], bds[4], bds[5]);
  //this->Widget3D->PlaceWidget(bounds);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PlaceWidget()
{
  // We should really check to see if the input has changed (modified).

  if (!this->Placed && this->Widget3DTclName)
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
    this->Script("pvRenderWindowInteractor Render");
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Use Label: " << (this->UseLabel?"on":"off") << endl;
  os << indent << "3D Widget:" << endl;
  if (this->Widget3DTclName)
    {
    os << indent << "3DWidgetTclName: " << this->Widget3DTclName << endl;
    }
}
