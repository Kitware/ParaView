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

#include "vtkCommand.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabeledFrame.h"
#include "vtk3DWidget.h"
#include "vtkObjectFactory.h"
#include "vtkPVApplication.h"
#include "vtkPVData.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkPVSource.h"
#include "vtkPVWindow.h"
#include "vtkPVXMLElement.h"
#include "vtkKWFrame.h"

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
  this->Widget3D = 0;
  this->Visible = 0;
  this->Placed = 0;
}

//----------------------------------------------------------------------------
vtkPV3DWidget::~vtkPV3DWidget()
{
  if ( this->Widget3D )
    {
    if (this->Widget3D->GetEnabled())
      {
      this->Widget3D->EnabledOff();
      }
    this->Widget3D->Delete();
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

  this->LabeledFrame->SetParent(this);
  this->LabeledFrame->Create(pvApp);
  this->LabeledFrame->SetLabel("3D Widget");

  this->Script("pack %s -fill both -expand 1", 
	       this->LabeledFrame->GetWidgetName());

  this->Frame->SetParent(this->LabeledFrame->GetFrame());
  this->Frame->Create(pvApp, 0);
  this->Script("pack %s -fill both -expand 1", 
	       this->Frame->GetWidgetName());
  
  this->Visibility->SetParent(this->LabeledFrame->GetFrame());
  this->Visibility->Create(pvApp, "");
  this->Visibility->SetText("Visibility");
  this->Visibility->SetCommand(this, "SetVisibility");
    
  this->Script("pack %s -fill x -expand 1",
	       this->Visibility->GetWidgetName());

  this->ChildCreate(pvApp);

  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if (iren)
    {
    this->Widget3D->SetInteractor(iren);
    this->Widget3D->AddObserver(vtkCommand::InteractionEvent, 
				this->Observer);
    this->Widget3D->AddObserver(vtkCommand::PlaceWidgetEvent, 
				this->Observer);
    this->Widget3D->EnabledOff();
    }
  this->Observer->Execute(this->Widget3D, vtkCommand::InteractionEvent, 0);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::CopyProperties(vtkPVWidget* clone, 
				   vtkPVSource* pvSource,
				   vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Reset()
{
  this->ModifiedFlag = 0;
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::Accept()
{
  this->PlaceWidget();
  this->ModifiedFlag = 0;
  this->ValueChanged = 0;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetValueChanged()
{
  this->ValueChanged = 1;
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
  this->Widget3D->SetEnabled(visibility);
  this->AddTraceEntry("$kw(%s) SetVisibility %d", 
		      this->GetTclName(), visibility);
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
  this->Widget3D->SetEnabled(visibility);  
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::SetFrameLabel(const char* label)
{
  if ( this->LabeledFrame )
    {
    this->LabeledFrame->SetLabel(label);
    } 
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ActualPlaceWidget()
{
  vtkDataSet* data = 0;
  if ( this->PVSource->GetPVInput() )
    {
    data = this->PVSource->GetPVInput()->GetVTKData();
    }
  this->Widget3D->SetInput(data);
  this->Widget3D->PlaceWidget();  
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PlaceWidget()
{
  vtkDataSet* data = 0;
  if ( this->PVSource->GetPVInput() )
    {
    data = this->PVSource->GetPVInput()->GetVTKData();
    }
  if (!this->Placed || data != this->Widget3D->GetInput())
    {
    this->ActualPlaceWidget();
    this->Placed = 1;
    this->ModifiedFlag = 1;
    }
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::ExecuteEvent(vtkObject*, unsigned long event, void*)
{
  if ( event != vtkCommand::PlaceWidgetEvent )
    {
    this->ModifiedCallback();
    }
}

//----------------------------------------------------------------------------
int vtkPV3DWidget::ReadXMLAttributes(vtkPVXMLElement*,
				     vtkPVXMLPackageParser*)
{
  return 1;
}

//----------------------------------------------------------------------------
void vtkPV3DWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << "3D Widget:" << endl;
  this->Widget3D->PrintSelf(os, indent.GetNextIndent());
}
