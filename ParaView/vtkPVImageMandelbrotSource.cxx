/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageMandelbrotSource.cxx
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

#include "vtkPVImageMandelbrotSource.h"
#include "vtkPVApplication.h"
#include "vtkPVImage.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

//----------------------------------------------------------------------------
vtkPVImageMandelbrotSource::vtkPVImageMandelbrotSource()
{
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->DimensionsFrame = vtkKWLabeledFrame::New();
  this->DimensionsFrame->SetParent(this->Properties);
  this->XDimension = vtkKWLabeledEntry::New();
  this->XDimension->SetParent(this->DimensionsFrame->GetFrame());
  this->YDimension = vtkKWLabeledEntry::New();
  this->YDimension->SetParent(this->DimensionsFrame->GetFrame());
  this->ZDimension = vtkKWLabeledEntry::New();
  this->ZDimension->SetParent(this->DimensionsFrame->GetFrame());

  this->CenterFrame = vtkKWLabeledFrame::New();
  this->CenterFrame->SetParent(this->Properties);
  this->CRealEntry = vtkKWLabeledEntry::New();
  this->CRealEntry->SetParent(this->CenterFrame->GetFrame());
  this->CImaginaryEntry = vtkKWLabeledEntry::New();
  this->CImaginaryEntry->SetParent(this->CenterFrame->GetFrame());
  this->XRealEntry = vtkKWLabeledEntry::New();
  this->XRealEntry->SetParent(this->CenterFrame->GetFrame());
  this->XImaginaryEntry = vtkKWLabeledEntry::New();
  this->XImaginaryEntry->SetParent(this->CenterFrame->GetFrame());
  
  this->CSpacingEntry = vtkKWLabeledEntry::New();
  this->CSpacingEntry->SetParent(this->Properties);
  
  this->XSpacingEntry = vtkKWLabeledEntry::New();
  this->XSpacingEntry->SetParent(this->Properties);
  
  vtkImageMandelbrotSource *s = vtkImageMandelbrotSource::New();
  s->SetMaximumNumberOfIterations(200);
  this->SetImageSource(s);
  s->Delete();
}

//----------------------------------------------------------------------------
vtkPVImageMandelbrotSource::~vtkPVImageMandelbrotSource()
{
  this->Accept->Delete();
  this->Accept = NULL;

  this->DimensionsFrame->Delete();
  this->XDimension->Delete();
  this->XDimension = NULL;
  this->YDimension->Delete();
  this->YDimension = NULL;
  this->ZDimension->Delete();
  this->ZDimension = NULL;
  
  this->CenterFrame->Delete();
  this->CRealEntry->Delete();
  this->CImaginaryEntry->Delete();
  this->XRealEntry->Delete();
  this->XImaginaryEntry->Delete();
  
  this->CSpacingEntry->Delete();  

  this->XSpacingEntry->Delete();  
}

//----------------------------------------------------------------------------
vtkPVImageMandelbrotSource* vtkPVImageMandelbrotSource::New()
{
  return new vtkPVImageMandelbrotSource();
}

//----------------------------------------------------------------------------
void vtkPVImageMandelbrotSource::CreateProperties()
{  
  this->vtkPVSource::CreateProperties();
  
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "AcceptParameters");
  this->Script("pack %s", this->Accept->GetWidgetName());
  
  this->DimensionsFrame->Create(this->Application);
  this->DimensionsFrame->SetLabel("Dimensions");
  this->Script("pack %s", this->DimensionsFrame->GetWidgetName());
  
  this->XDimension->Create(this->Application);
  this->Script("%s configure -width 4",
	       this->XDimension->GetEntry()->GetWidgetName());
  this->XDimension->SetLabel("X:");
  this->YDimension->Create(this->Application);
  this->Script("%s configure -width 4",
	       this->YDimension->GetEntry()->GetWidgetName());
  this->YDimension->SetLabel("Y:");
  this->ZDimension->Create(this->Application);
  this->Script("%s configure -width 4");
  this->ZDimension->SetLabel("Z:");
  this->Script("pack %s %s %s -side left", 
	       this->XDimension->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZDimension->GetWidgetName());

  this->CenterFrame->Create(this->Application);
  this->CenterFrame->SetLabel("Center Parameters");
  this->Script("pack %s", this->CenterFrame->GetWidgetName());
  
  this->CRealEntry->Create(this->Application);
  this->Script("%s configure -width 7",
	       this->CRealEntry->GetEntry()->GetWidgetName());
  this->CRealEntry->SetLabel("C: ");
  this->CRealEntry->SetValue(0.0);
  this->CImaginaryEntry->Create(this->Application);
  this->Script("%s configure -width 7",
	       this->CImaginaryEntry->GetEntry()->GetWidgetName());
  this->CImaginaryEntry->SetLabel(" + i");
  this->CImaginaryEntry->SetValue(0.0);  
  this->XRealEntry->Create(this->Application);
  this->Script("%s configure -width 7",
	       this->XRealEntry->GetEntry()->GetWidgetName());
  this->XRealEntry->SetLabel("X: ");
  this->XRealEntry->SetValue(0.0);
  this->XImaginaryEntry->Create(this->Application);
  this->Script("%s configure -width 7",
	       this->XImaginaryEntry->GetEntry()->GetWidgetName());
  this->XImaginaryEntry->SetLabel(" + i");
  this->XImaginaryEntry->SetValue(0.0);
  
  this->Script("pack %s %s %s %s -side left", 
	       this->CRealEntry->GetWidgetName(),
	       this->CImaginaryEntry->GetWidgetName(),
	       this->XRealEntry->GetWidgetName(),
	       this->XImaginaryEntry->GetWidgetName());
  
  this->CSpacingEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->CSpacingEntry->GetEntry()->GetWidgetName());
  this->CSpacingEntry->SetLabel("C Spacing: ");
  this->CSpacingEntry->SetValue(0.1);
  
  this->XSpacingEntry->Create(this->Application);
  this->Script("%s configure -width 12",
	       this->XSpacingEntry->GetEntry()->GetWidgetName());
  this->XSpacingEntry->SetLabel("X Spacing: ");
  this->XSpacingEntry->SetValue(0.1);
  
  this->Script("pack %s %s", 
	       this->CSpacingEntry->GetWidgetName(),
	       this->XSpacingEntry->GetWidgetName());
  
  // initialize parameters
  this->SetCenter(-0.733569, 0.24405, 0.296116, 0.0253163);
  this->SetSpacing(1.38125e-005, 1.0e-004);
  this->SetDimensions(110, 110, 110);
}

//----------------------------------------------------------------------------
void vtkPVImageMandelbrotSource::SetDimensions(int dx, int dy, int dz)
{
  int mx, my, mz;
  vtkPVApplication *pvApp = this->GetPVApplication();

  mx = dx/2;
  my = dy/2;
  mz = dz/2;
  
  this->GetImageMandelbrotSource()->SetWholeExtent(-mx, dx-mx-1,
						   -my, dy-my-1,
						   -mz, dz-mz-1);
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    this->XDimension->SetValue(dx);
    this->YDimension->SetValue(dy);
    this->ZDimension->SetValue(dz);
    pvApp->BroadcastScript("%s SetDimensions %d %d %d", 
			   this->GetTclName(), dx, dy, dz);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageMandelbrotSource::SetSpacing(float sc, float sx)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->GetImageMandelbrotSource()->SetSampleCX(sc, sc, sx, sx);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    this->CSpacingEntry->SetValue(sc, 10);
    this->XSpacingEntry->SetValue(sx, 10);
    pvApp->BroadcastScript("%s SetSpacing %f %f", this->GetTclName(), sc, sx);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageMandelbrotSource::SetCenter(float cr, float ci, 
					   float xr, float xi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->GetImageMandelbrotSource()->SetOriginCX(cr, ci, xr, xi);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    // Keep the UI up to date in case this is being called externally.
    this->CRealEntry->SetValue(cr, 8);
    this->CImaginaryEntry->SetValue(ci, 8);
    this->XRealEntry->SetValue(xr, 8);
    this->XImaginaryEntry->SetValue(xi, 8);
    pvApp->BroadcastScript("%s SetCenter %f %f %f %f",
			   this->GetTclName(), cr, ci, xr, xi);
    }
}

//----------------------------------------------------------------------------
vtkImageMandelbrotSource *vtkPVImageMandelbrotSource::GetImageMandelbrotSource()
{
  return vtkImageMandelbrotSource::SafeDownCast(this->ImageSource);  
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartImageMandelbrotSourceProgress(void *arg)
{
  vtkPVImageMandelbrotSource *me = (vtkPVImageMandelbrotSource*)arg;
  me->GetWindow()->SetStatusText("Processing ImageMandelbrotSource");
}

//----------------------------------------------------------------------------
void ImageMandelbrotSourceProgress(void *arg)
{
  vtkPVImageMandelbrotSource *me = (vtkPVImageMandelbrotSource*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetImageMandelbrotSource()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndImageMandelbrotSourceProgress(void *arg)
{
  vtkPVImageMandelbrotSource *me = (vtkPVImageMandelbrotSource*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void vtkPVImageMandelbrotSource::AcceptParameters()
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetWindow();
  
  // Setup the image reader from the UI.
  this->SetDimensions(this->XDimension->GetValueAsInt(),
		      this->YDimension->GetValueAsInt(),
		      this->ZDimension->GetValueAsInt());
  this->SetSpacing(this->CSpacingEntry->GetValueAsFloat(), 
		   this->XSpacingEntry->GetValueAsFloat());
  this->SetCenter(this->CRealEntry->GetValueAsFloat(),
		  this->CImaginaryEntry->GetValueAsFloat(),
		  this->XRealEntry->GetValueAsFloat(),
		  this->XImaginaryEntry->GetValueAsFloat());

  if (this->GetPVData() == NULL)
    {
    this->GetImageMandelbrotSource()->
      SetStartMethod(StartImageMandelbrotSourceProgress, this);
    this->GetImageMandelbrotSource()->
      SetProgressMethod(ImageMandelbrotSourceProgress, this);
    this->GetImageMandelbrotSource()->
      SetEndMethod(EndImageMandelbrotSourceProgress, this);
    this->InitializeData();
    }

  if (window->GetPreviousSource() != NULL)
    {
    window->GetPreviousSource()->GetPVData()->GetActorComposite()->VisibilityOff();
    }
  
  window->GetMainView()->SetSelectedComposite(this);
  window->GetMainView()->ResetCamera();
  this->GetView()->Render();
}
