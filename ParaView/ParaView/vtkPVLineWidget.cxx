/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVLineWidget.cxx
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
#include "vtkPVLineWidget.h"
#include "vtkPVApplication.h"
#include "vtkPVXMLElement.h"
#include "vtkPVData.h"
#include "vtkPVWindow.h"

#include "vtkKWEntry.h"
#include "vtkKWFrame.h"

#include "vtkDataSet.h"
#include "vtkPVGenericRenderWindowInteractor.h"
#include "vtkLineWidget.h"
#include "vtkCommand.h"

#include "vtkObjectFactory.h"

//===========================================================================
//***************************************************************************
struct vtkLineWidgetObserver : public vtkCommand
{
  static vtkLineWidgetObserver *New() 
    {return new vtkLineWidgetObserver;};

  vtkLineWidgetObserver()
    {
      this->PVLineWidget = 0;
    }

  virtual void Execute(vtkObject* wdg, unsigned long, 
                       void*)
    {
      cout << "Event invoked." << endl;
      vtkLineWidget* widget = vtkLineWidget::SafeDownCast(wdg);
      if (!widget)
	{
	return;
	}
      if (this->PVLineWidget)
	{
	float val[3];
	int i;
	widget->GetPoint1(val);
	for (i=0; i<3; i++)
	  {
	  this->PVLineWidget->Point1[i]->SetValue(val[i],5);
	  }
	widget->GetPoint2(val);
	for (i=0; i<3; i++)
	  {
	  this->PVLineWidget->Point2[i]->SetValue(val[i],5);
	  }
	}
    }

  vtkPVLineWidget* PVLineWidget;
};
//***************************************************************************
//===========================================================================

//----------------------------------------------------------------------------
vtkPVLineWidget* vtkPVLineWidget::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPVLineWidget");
  if (ret)
    {
    return (vtkPVLineWidget*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPVLineWidget;
}

//----------------------------------------------------------------------------
vtkPVLineWidget::vtkPVLineWidget()
{
  this->Widget3D = vtkLineWidget::New();
  this->Observer = vtkLineWidgetObserver::New();
  this->Observer->PVLineWidget = this;
  for (int i=0; i<3; i++)
    {
    this->Point1[i] = vtkKWEntry::New();
    this->Point2[i] = vtkKWEntry::New();
    }
}

//----------------------------------------------------------------------------
vtkPVLineWidget::~vtkPVLineWidget()
{
  cerr << "In destructor" << endl;
  if (this->Widget3D->GetEnabled())
    {
    cerr << "Disabling" << endl;
//    this->Widget3D->EnabledOff();
    }
  this->Widget3D->Delete();
  this->Observer->Delete();
  for (int i=0; i<3; i++)
    {
    this->Point1[i]->Delete();
    this->Point2[i]->Delete();
    }
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::Create(vtkKWApplication *pvApp)
{
  const char* wname;
  
  if (this->Application)
    {
    vtkErrorMacro("LineWidget already created");
    return;
    }

  this->SetApplication(pvApp);
  
  // create the top level
  wname = this->GetWidgetName();
  this->Script("frame %s -borderwidth 0 -relief flat", wname);

//  vtkKWFrame* frame = vtkKWFrame::New();
//  frame->SetParent(this);
//  frame->Create(pvApp, 0);
//  frame->UnRegister(this);

  int i;
  for (i=0; i<3; i++)
    {
    this->Point1[i]->SetParent(this);
    this->Point1[i]->Create(pvApp, "");
    
    this->Point2[i]->SetParent(this);
    this->Point2[i]->Create(pvApp, "");
    }
    
  this->Script("grid %s %s %s -sticky ew",
	       this->Point1[0]->GetWidgetName(),
	       this->Point1[1]->GetWidgetName(),
	       this->Point1[2]->GetWidgetName());
  this->Script("grid %s %s %s -sticky ew",
	       this->Point2[0]->GetWidgetName(),
	       this->Point2[1]->GetWidgetName(),
	       this->Point2[2]->GetWidgetName());
  this->Script("grid columnconfigure %s 0 -weight 1", this->GetWidgetName());
  this->Script("grid columnconfigure %s 1 -weight 1", this->GetWidgetName());
  this->Script("grid columnconfigure %s 2 -weight 1", this->GetWidgetName());

//  this->Script("pack %s -side top", frame->GetWidgetName());

  for (i=0; i<3; i++)
    {
    this->Script("bind %s <KeyPress-Return> {%s SetPoint1}",
		 this->Point1[i]->GetWidgetName(),
		 this->GetTclName());
    this->Script("bind %s <KeyPress-Return> {%s SetPoint2}",
		 this->Point2[i]->GetWidgetName(),
		 this->GetTclName());
    }

  vtkDataSet* data = this->PVSource->GetPVOutput()->GetVTKData();
  if (data)
    {
    this->Widget3D->SetInput(data);
    this->Widget3D->PlaceWidget();
    }
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if (iren)
    {
    this->Widget3D->SetInteractor(iren);
    this->Widget3D->AddObserver(vtkCommand::InteractionEvent, 
				this->Observer);
    this->Widget3D->EnabledOn();
    }
  this->Observer->Execute(this->Widget3D, vtkCommand::InteractionEvent, 0);
}


//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint1()
{
  float pos[3];
  int i;
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point1[i]->GetValueAsFloat();
    }
  this->Widget3D->SetPoint1(pos);
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::SetPoint2()
{
  float pos[3];
  int i;
  for (i=0; i<3; i++)
    {
    pos[i] = this->Point2[i]->GetValueAsFloat();
    }
  this->Widget3D->SetPoint2(pos);
  vtkPVGenericRenderWindowInteractor* iren = 
    this->PVSource->GetPVWindow()->GetGenericInteractor();
  if(iren)
    {
    iren->Render();
    }
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::Accept()
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->ModifiedFlag = 0;
}

//----------------------------------------------------------------------------
void vtkPVLineWidget::Reset()
{
  if ( ! this->ModifiedFlag)
    {
    return;
    }

  this->ModifiedFlag = 0;
}

vtkPVLineWidget* vtkPVLineWidget::ClonePrototype(vtkPVSource* pvSource,
				 vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  vtkPVWidget* clone = this->ClonePrototypeInternal(pvSource, map);
  return vtkPVLineWidget::SafeDownCast(clone);
}

void vtkPVLineWidget::CopyProperties(vtkPVWidget* clone, 
				      vtkPVSource* pvSource,
			      vtkArrayMap<vtkPVWidget*, vtkPVWidget*>* map)
{
  this->Superclass::CopyProperties(clone, pvSource, map);
  vtkPVLineWidget* pvlw = vtkPVLineWidget::SafeDownCast(clone);
  if (pvlw)
    {
    }
  else 
    {
    vtkErrorMacro("Internal error. Could not downcast clone to PVLineWidget.");
    }
}

//----------------------------------------------------------------------------
int vtkPVLineWidget::ReadXMLAttributes(vtkPVXMLElement* element,
                                        vtkPVXMLPackageParser* parser)
{
  if(!this->Superclass::ReadXMLAttributes(element, parser)) { return 0; }

  return 1;
}
