/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVContourFilter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-2000 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "vtkPVContourFilter.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"


int vtkPVContourFilterCommand(ClientData cd, Tcl_Interp *interp,
			      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVContourFilter::vtkPVContourFilter()
{
  this->CommandFunction = vtkPVContourFilterCommand;
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this->Properties);
  this->ContourValueEntry = vtkKWEntry::New();
  this->ContourValueEntry->SetParent(this->Properties);
  this->ContourValueLabel = vtkKWLabel::New();
  this->ContourValueLabel->SetParent(this->Properties);
  
  this->Contour = vtkContourFilter::New();  
}

//----------------------------------------------------------------------------
vtkPVContourFilter::~vtkPVContourFilter()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ContourValueEntry->Delete();
  this->ContourValueEntry = NULL;
  this->ContourValueLabel->Delete();
  this->ContourValueLabel = NULL;
  
  this->Contour->Delete();
  this->Contour = NULL;
}

//----------------------------------------------------------------------------
vtkPVContourFilter* vtkPVContourFilter::New()
{
  return new vtkPVContourFilter();
}

//----------------------------------------------------------------------------
void vtkPVContourFilter::CreateProperties()
{
  this->vtkPVSource::CreateProperties();
  
  this->ContourValueLabel->Create(this->Application, "");
  this->ContourValueLabel->SetLabel("Contour Value:");
  this->ContourValueEntry->Create(this->Application, "");
  this->ContourValueEntry->SetValue(this->GetContour()->GetValue(0), 2);
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ContourValueChanged");
  this->Script("pack %s", this->Accept->GetWidgetName());
  this->Script("pack %s %s -side left -anchor w",
	       this->ContourValueLabel->GetWidgetName(),
	       this->ContourValueEntry->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVContourFilter::SetInput(vtkPVData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetContour()->SetInput(pvData->GetData());
}



//----------------------------------------------------------------------------
void vtkPVContourFilter::SetOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(),
			   pvd->GetTclName());
    }  
  
  this->SetPVData(pvd);  
  pvd->SetPolyData(this->Contour->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVContourFilter::GetOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVContourFilter::SetValue(int contour, float val)
{  
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetValue %d %f", this->GetTclName(),
			   contour, val);
    }
  
  this->GetContour()->SetValue(contour, val);
}

//----------------------------------------------------------------------------
void vtkPVContourFilter::ContourValueChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVWindow *window = this->GetWindow();
  vtkPVPolyData *pvd;
  vtkPVAssignment *a;
  vtkPVActorComposite *ac;
  
  this->SetValue(0, this->ContourValueEntry->GetValueAsFloat());

  if (this->GetPVData() == NULL)
    { // This is the first time.  Create the data.
    pvd = vtkPVPolyData::New();
    pvd->Clone(pvApp);
    this->SetOutput(pvd);
    a = window->GetPreviousSource()->GetPVData()->GetAssignment();
    pvd->SetAssignment(a);
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff(); 
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
}


