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
  this->LowPointXEntry = vtkKWLabeledEntry::New();
  this->LowPointXEntry->SetParent(this->LowPointFrame);
  this->LowPointYEntry = vtkKWLabeledEntry::New();
  this->LowPointYEntry->SetParent(this->LowPointFrame);
  this->LowPointZEntry = vtkKWLabeledEntry::New();
  this->LowPointZEntry->SetParent(this->LowPointFrame);

  this->HighPointLabel = vtkKWLabel::New();
  this->HighPointLabel->SetParent(this->Properties);
  this->HighPointFrame = vtkKWWidget::New();
  this->HighPointFrame->SetParent(this->Properties);
  this->HighPointXEntry = vtkKWLabeledEntry::New();
  this->HighPointXEntry->SetParent(this->HighPointFrame);
  this->HighPointYEntry = vtkKWLabeledEntry::New();
  this->HighPointYEntry->SetParent(this->HighPointFrame);
  this->HighPointZEntry = vtkKWLabeledEntry::New();
  this->HighPointZEntry->SetParent(this->HighPointFrame);

  this->RangeLabel = vtkKWLabel::New();
  this->RangeLabel->SetParent(this->Properties);
  this->RangeFrame = vtkKWWidget::New();
  this->RangeFrame->SetParent(this->Properties);
  this->RangeMinEntry = vtkKWLabeledEntry::New();
  this->RangeMinEntry->SetParent(this->RangeFrame);
  this->RangeMaxEntry = vtkKWLabeledEntry::New();
  this->RangeMaxEntry->SetParent(this->RangeFrame);

  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  
  vtkElevationFilter *e = vtkElevationFilter::New();
  this->SetFilter(e);
  e->Delete();
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
  
  this->LowPointFrame->Delete();
  this->LowPointFrame = NULL;
  this->HighPointFrame->Delete();
  this->HighPointFrame = NULL;
  this->RangeFrame->Delete();
  this->RangeFrame = NULL;
  
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->SourceButton->Delete();
  this->SourceButton = NULL;
}

//----------------------------------------------------------------------------
vtkPVElevationFilter* vtkPVElevationFilter::New()
{
  return new vtkPVElevationFilter();
}

//----------------------------------------------------------------------------
vtkElevationFilter* vtkPVElevationFilter::GetElevation()
{
  return vtkElevationFilter::SafeDownCast(this->Filter);
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

  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Script("pack %s", this->SourceButton->GetWidgetName());
  
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ElevationParameterChanged");

  this->LowPointLabel->Create(this->Application, "-pady 6");
  this->LowPointLabel->SetLabel("LowPoint");
  this->LowPointFrame->Create(this->Application, "frame", "-bd 0");
  this->LowPointXEntry->Create(this->Application);
  this->LowPointXEntry->SetLabel("X:");
  this->LowPointXEntry->SetValue(low[0], 2);
  this->Script("%s configure -width 10",
	       this->LowPointXEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->LowPointXEntry->GetWidgetName());
  this->LowPointYEntry->Create(this->Application);
  this->LowPointYEntry->SetLabel("Y:");
  this->LowPointYEntry->SetValue(low[1], 2);
  this->Script("%s configure -width 10",
	       this->LowPointYEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->LowPointYEntry->GetWidgetName());
  this->LowPointZEntry->Create(this->Application);
  this->LowPointZEntry->SetLabel("Z:");
  this->LowPointZEntry->SetValue(low[2], 2);
  this->Script("%s configure -width 10",
	       this->LowPointZEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->LowPointZEntry->GetWidgetName());
  this->Script("pack %s %s %s",
	       this->Accept->GetWidgetName(),
	       this->LowPointLabel->GetWidgetName(),
	       this->LowPointFrame->GetWidgetName());
  
  this->HighPointFrame->Create(this->Application, "frame", "-bd 0");
  this->HighPointLabel->Create(this->Application, "-pady 6");
  this->HighPointLabel->SetLabel("High Point");
  this->HighPointXEntry->Create(this->Application);
  this->HighPointXEntry->SetLabel("X:");
  this->HighPointXEntry->SetValue(high[0], 2);
  this->Script("%s configure -width 10",
	       this->HighPointXEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->HighPointXEntry->GetWidgetName());
  this->HighPointYEntry->Create(this->Application);
  this->HighPointYEntry->SetLabel("Y:");
  this->HighPointYEntry->SetValue(high[1], 2);
  this->Script("%s configure -width 10",
	       this->HighPointYEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->HighPointYEntry->GetWidgetName());
  this->HighPointZEntry->Create(this->Application);
  this->HighPointZEntry->SetLabel("Z:");
  this->HighPointZEntry->SetValue(high[2], 2);
  this->Script("%s configure -width 10",
	       this->HighPointZEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->HighPointZEntry->GetWidgetName());
  this->Script("pack %s %s",
	       this->HighPointLabel->GetWidgetName(),
	       this->HighPointFrame->GetWidgetName());
  
  this->RangeFrame->Create(this->Application, "frame", "-bd 0");
  this->RangeLabel->Create(this->Application, "-pady 6");
  this->RangeLabel->SetLabel("Scalar Range");
  this->RangeMinEntry->Create(this->Application);
  this->RangeMinEntry->SetLabel("Min.:");
  this->RangeMinEntry->SetValue(range[0], 2);
  this->Script("%s configure -width 10",
	       this->RangeMinEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->RangeMinEntry->GetWidgetName());
  this->RangeMaxEntry->Create(this->Application);
  this->RangeMaxEntry->SetLabel("Max.:");
  this->RangeMaxEntry->SetValue(range[1], 2);
  this->Script("%s configure -width 10",
	       this->RangeMaxEntry->GetEntry()->GetWidgetName());
  this->Script("pack %s -side left -anchor w -expand yes",
	       this->RangeMaxEntry->GetWidgetName());
  this->Script("pack %s %s",
	       this->RangeLabel->GetWidgetName(),
	       this->RangeFrame->GetWidgetName());
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartElevationFilterProgress(void *arg)
{
  vtkPVElevationFilter *me = (vtkPVElevationFilter*)arg;
  me->GetWindow()->SetStatusText("Processing Elevation Filter");
}

//----------------------------------------------------------------------------
void ElevationFilterProgress(void *arg)
{
  vtkPVElevationFilter *me = (vtkPVElevationFilter*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetElevation()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndElevationFilterProgress(void *arg)
{
  vtkPVElevationFilter *me = (vtkPVElevationFilter*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVElevationFilter::ElevationParameterChanged()
{
  vtkPVWindow *window = this->GetWindow();
  float low[3], high[3], range[2];
  
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
    this->GetElevation()->SetStartMethod(StartElevationFilterProgress, this);
    this->GetElevation()->SetProgressMethod(ElevationFilterProgress, this);
    this->GetElevation()->SetEndMethod(EndElevationFilterProgress, this);
    this->InitializeData();
    window->GetSourceList()->Update();
    }
  
  this->GetView()->Render();
  window->GetMainView()->SetSelectedComposite(this);
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

//----------------------------------------------------------------------------
void vtkPVElevationFilter::GetSource()
{
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetInput()->GetPVSource());
  this->GetInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}
