/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVElevationFilter.cxx
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

#include "vtkPVElevationFilter.h"
#include "vtkPVApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVPolyData.h"
#include "vtkPVImage.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"

int vtkPVElevationFilterCommand(ClientData cd, Tcl_Interp *interp,
				int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVElevationFilter::vtkPVElevationFilter()
{
  this->CommandFunction = vtkPVElevationFilterCommand;
  
  this->LowPointLabel = vtkKWLabel::New();
  this->LowPointLabel->SetParent(this->Properties);
  this->LowPointFrame = vtkKWWidget::New();
  this->LowPointFrame->SetParent(this->Properties);
  this->LowPointXEntry = vtkKWEntry::New();
  this->LowPointXEntry->SetParent(this->LowPointFrame);
  this->LowPointYEntry = vtkKWEntry::New();
  this->LowPointYEntry->SetParent(this->LowPointFrame);
  this->LowPointZEntry = vtkKWEntry::New();
  this->LowPointZEntry->SetParent(this->LowPointFrame);

  this->HighPointLabel = vtkKWLabel::New();
  this->HighPointLabel->SetParent(this->Properties);
  this->HighPointFrame = vtkKWWidget::New();
  this->HighPointFrame->SetParent(this->Properties);
  this->HighPointXEntry = vtkKWEntry::New();
  this->HighPointXEntry->SetParent(this->HighPointFrame);
  this->HighPointYEntry = vtkKWEntry::New();
  this->HighPointYEntry->SetParent(this->HighPointFrame);
  this->HighPointZEntry = vtkKWEntry::New();
  this->HighPointZEntry->SetParent(this->HighPointFrame);

  this->RangeLabel = vtkKWLabel::New();
  this->RangeLabel->SetParent(this->Properties);
  this->RangeFrame = vtkKWWidget::New();
  this->RangeFrame->SetParent(this->Properties);
  this->RangeMinEntry = vtkKWEntry::New();
  this->RangeMinEntry->SetParent(this->RangeFrame);
  this->RangeMaxEntry = vtkKWEntry::New();
  this->RangeMaxEntry->SetParent(this->RangeFrame);

  this->LowPointXLabel = vtkKWLabel::New();
  this->LowPointXLabel->SetParent(this->LowPointFrame);
  this->LowPointYLabel = vtkKWLabel::New();
  this->LowPointYLabel->SetParent(this->LowPointFrame);
  this->LowPointZLabel = vtkKWLabel::New();
  this->LowPointZLabel->SetParent(this->LowPointFrame);
  this->HighPointXLabel = vtkKWLabel::New();
  this->HighPointXLabel->SetParent(this->HighPointFrame);
  this->HighPointYLabel = vtkKWLabel::New();
  this->HighPointYLabel->SetParent(this->HighPointFrame);
  this->HighPointZLabel = vtkKWLabel::New();
  this->HighPointZLabel->SetParent(this->HighPointFrame);
  this->RangeMinLabel = vtkKWLabel::New();
  this->RangeMinLabel->SetParent(this->RangeFrame);
  this->RangeMaxLabel = vtkKWLabel::New();
  this->RangeMaxLabel->SetParent(this->RangeFrame);
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this->Properties);
  
  this->Elevation = vtkElevationFilter::New();

}

//----------------------------------------------------------------------------
vtkPVElevationFilter::~vtkPVElevationFilter()
{
  this->LowPointLabel->Delete();
  this->LowPointLabel = NULL;
  this->LowPointXEntry->Delete();
  this->LowPointXEntry = NULL;
  this->LowPointYEntry->Delete();
  this->LowPointYEntry = NULL;
  this->LowPointZEntry->Delete();
  this->LowPointZEntry = NULL;
  this->HighPointLabel->Delete();
  this->HighPointLabel = NULL;
  this->HighPointXEntry->Delete();
  this->HighPointXEntry = NULL;
  this->HighPointYEntry->Delete();
  this->HighPointYEntry = NULL;
  this->HighPointZEntry->Delete();
  this->HighPointZEntry = NULL;
  this->RangeLabel->Delete();
  this->RangeLabel = NULL;
  this->RangeMinEntry->Delete();
  this->RangeMinEntry = NULL;
  this->RangeMaxEntry->Delete();
  this->RangeMaxEntry = NULL;
  
  this->LowPointXLabel->Delete();
  this->LowPointXLabel = NULL;
  this->LowPointYLabel->Delete();
  this->LowPointYLabel = NULL;
  this->LowPointZLabel->Delete();
  this->LowPointZLabel = NULL;
  this->HighPointXLabel->Delete();
  this->HighPointXLabel = NULL;
  this->HighPointYLabel->Delete();
  this->HighPointYLabel = NULL;
  this->HighPointZLabel->Delete();
  this->HighPointZLabel = NULL;
  this->RangeMinLabel->Delete();
  this->RangeMinLabel = NULL;
  this->RangeMaxLabel->Delete();
  this->RangeMaxLabel = NULL;

  this->LowPointFrame->Delete();
  this->LowPointFrame = NULL;
  this->HighPointFrame->Delete();
  this->HighPointFrame = NULL;
  this->RangeFrame->Delete();
  this->RangeFrame = NULL;
  
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->Elevation->Delete();
  this->Elevation = NULL;
}

//----------------------------------------------------------------------------
vtkPVElevationFilter* vtkPVElevationFilter::New()
{
  return new vtkPVElevationFilter();
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::CreateProperties()
{ 
  float *low, *high, *range;
 
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  low = this->GetElevation()->GetLowPoint();
  high = this->GetElevation()->GetHighPoint();
  range = this->GetElevation()->GetScalarRange();

  this->Accept->Create(this->Application, "button",
		       "-text Accept");
  this->Accept->SetCommand(this, "ElevationParameterChanged");

  this->LowPointLabel->Create(this->Application, "-pady 6");
  this->LowPointLabel->SetLabel("LowPoint");
  this->LowPointFrame->Create(this->Application, "frame", "-bd 0");
  this->LowPointXLabel->Create(this->Application, "-padx 4");
  this->LowPointXLabel->SetLabel("X:");
  this->LowPointXEntry->Create(this->Application, "-width 10");
  this->LowPointXEntry->SetValue(low[0], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->LowPointXLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->LowPointXEntry->GetWidgetName());
  this->LowPointYLabel->Create(this->Application, "-padx 4");
  this->LowPointYLabel->SetLabel("Y:");
  this->LowPointYEntry->Create(this->Application, "-width 10");
  this->LowPointYEntry->SetValue(low[1], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->LowPointYLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->LowPointYEntry->GetWidgetName());
  this->LowPointZLabel->Create(this->Application, "-padx 4");
  this->LowPointZLabel->SetLabel("Z:");
  this->LowPointZEntry->Create(this->Application, "-width 10");
  this->LowPointZEntry->SetValue(low[2], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->LowPointZLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->LowPointZEntry->GetWidgetName());
  this->Script("pack %s %s %s",
	       this->Accept->GetWidgetName(),
	       this->LowPointLabel->GetWidgetName(),
	       this->LowPointFrame->GetWidgetName());
  
  this->HighPointFrame->Create(this->Application, "frame", "-bd 0");
  this->HighPointLabel->Create(this->Application, "-pady 6");
  this->HighPointLabel->SetLabel("High Point");
  this->HighPointXLabel->Create(this->Application, "-padx 4");
  this->HighPointXLabel->SetLabel("X:");
  this->HighPointXEntry->Create(this->Application, "-width 10");
  this->HighPointXEntry->SetValue(high[0], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->HighPointXLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->HighPointXEntry->GetWidgetName());
  this->HighPointYLabel->Create(this->Application, "-padx 4");
  this->HighPointYLabel->SetLabel("Y:");
  this->HighPointYEntry->Create(this->Application, "-width 10");
  this->HighPointYEntry->SetValue(high[1], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->HighPointYLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->HighPointYEntry->GetWidgetName());
  this->HighPointZLabel->Create(this->Application, "-padx 4");
  this->HighPointZLabel->SetLabel("Z:");
  this->HighPointZEntry->Create(this->Application, "-width 10");
  this->HighPointZEntry->SetValue(high[2], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->HighPointZLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->HighPointZEntry->GetWidgetName());
  this->Script("pack %s %s",
	       this->HighPointLabel->GetWidgetName(),
	       this->HighPointFrame->GetWidgetName());
  
  this->RangeFrame->Create(this->Application, "frame", "-bd 0");
  this->RangeLabel->Create(this->Application, "-pady 6");
  this->RangeLabel->SetLabel("Scalar Range");
  this->RangeMinLabel->Create(this->Application, "-padx 4");
  this->RangeMinLabel->SetLabel("Min.:");
  this->RangeMinEntry->Create(this->Application, "-width 10");
  this->RangeMinEntry->SetValue(range[0], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->RangeMinLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->RangeMinEntry->GetWidgetName());
  this->RangeMaxLabel->Create(this->Application, "-padx 4");
  this->RangeMaxLabel->SetLabel("Max.:");
  this->RangeMaxEntry->Create(this->Application, "-width 10");
  this->RangeMaxEntry->SetValue(range[1], 2);
  this->Script("pack %s -side left -anchor w -expand no",
	       this->RangeMaxLabel->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->RangeMaxEntry->GetWidgetName());
  this->Script("pack %s %s",
	       this->RangeLabel->GetWidgetName(),
	       this->RangeFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::SetOutput(vtkPVPolyData *pvd)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(), 
			   pvd->GetTclName());
    }
  
  this->SetPVData(pvd);
  pvd->SetPolyData(this->Elevation->GetPolyDataOutput());
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::SetOutput(vtkPVImage *pvd)
{
  vtkDataSet *ds;
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(), 
			   pvd->GetTclName());
    }
  
  this->SetPVData(pvd);
  
  ds = this->Elevation->GetOutput();
  if ( ! ds->IsA("vtkImageData") )
    {
    vtkErrorMacro("Expecting an image.");
    return;
    }
  
  pvd->SetImageData((vtkImageData*)ds);
}

//----------------------------------------------------------------------------
vtkPVData *vtkPVElevationFilter::GetOutput()
{
  return this->Output;
}

//----------------------------------------------------------------------------
vtkPVPolyData *vtkPVElevationFilter::GetPVPolyDataOutput()
{
  return vtkPVPolyData::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVElevationFilter::GetPVImageOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::ElevationParameterChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVPolyData *newData;
  vtkPVAssignment *a;
  vtkPVWindow *window = this->GetWindow();
  float low[3], high[3], range[2];
  vtkPVActorComposite *ac;
  
  low[0] = this->LowPointXEntry->GetValueAsFloat();
  low[1] = this->LowPointYEntry->GetValueAsFloat();
  low[2] = this->LowPointZEntry->GetValueAsFloat();
  high[0] = this->HighPointXEntry->GetValueAsFloat();
  high[1] = this->HighPointYEntry->GetValueAsFloat();
  high[2] = this->HighPointZEntry->GetValueAsFloat();
  range[0] = this->RangeMinEntry->GetValueAsFloat();
  range[1] = this->RangeMaxEntry->GetValueAsFloat();
  
  this->SetLowPoint(low[0], low[1], low[2]);
  this->SetHighPoint(high[0], high[1], high[2]);
  this->SetScalarRange(range[0], range[1]);

  if (this->GetPVData() == NULL)
    { // This is the first time. Initialize data.
    newData = vtkPVPolyData::New();
    newData->Clone(pvApp);
    this->SetOutput(newData);
    a = window->GetPreviousSource()->GetPVData()->GetAssignment();
    newData->SetAssignment(a);
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  
  this->GetView()->Render();
  window->GetMainView()->SetSelectedComposite(this);
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::SetInput(vtkPVData *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetElevation()->SetInput(pvData->GetData());
}


//----------------------------------------------------------------------------
void vtkPVElevationFilter::SetLowPoint(float x, float y, float z)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetLowPoint %f %f %f", this->GetTclName(),
			   x, y, z);
    }  
  
  this->GetElevation()->SetLowPoint(x, y, z);
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::SetHighPoint(float x, float y, float z)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetHighPoint %f %f %f", this->GetTclName(),
			   x, y, z);
    }  
  
  this->GetElevation()->SetHighPoint(x, y, z);
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::SetScalarRange(float min, float max)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetScalarRange %f %f", this->GetTclName(),
			   min, max);
    }  
  
  this->GetElevation()->SetScalarRange(min, max);
}
