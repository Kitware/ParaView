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
  this->XDimLabel = vtkKWLabel::New();
  this->XDimLabel->SetParent(this->DimensionsFrame->GetFrame());
  this->XDimension = vtkKWEntry::New();
  this->XDimension->SetParent(this->DimensionsFrame->GetFrame());
  this->YDimLabel = vtkKWLabel::New();
  this->YDimLabel->SetParent(this->DimensionsFrame->GetFrame());
  this->YDimension = vtkKWEntry::New();
  this->YDimension->SetParent(this->DimensionsFrame->GetFrame());
  this->ZDimLabel = vtkKWLabel::New();
  this->ZDimLabel->SetParent(this->DimensionsFrame->GetFrame());
  this->ZDimension = vtkKWEntry::New();
  this->ZDimension->SetParent(this->DimensionsFrame->GetFrame());

  this->CenterFrame = vtkKWLabeledFrame::New();
  this->CenterFrame->SetParent(this->Properties);
  this->CRealLabel = vtkKWLabel::New();
  this->CRealLabel->SetParent(this->CenterFrame->GetFrame());
  this->CRealEntry = vtkKWEntry::New();
  this->CRealEntry->SetParent(this->CenterFrame->GetFrame());
  this->CImaginaryLabel = vtkKWLabel::New();
  this->CImaginaryLabel->SetParent(this->CenterFrame->GetFrame());
  this->CImaginaryEntry = vtkKWEntry::New();
  this->CImaginaryEntry->SetParent(this->CenterFrame->GetFrame());
  this->XRealLabel = vtkKWLabel::New();
  this->XRealLabel->SetParent(this->CenterFrame->GetFrame());
  this->XRealEntry = vtkKWEntry::New();
  this->XRealEntry->SetParent(this->CenterFrame->GetFrame());
  this->XImaginaryLabel = vtkKWLabel::New();
  this->XImaginaryLabel->SetParent(this->CenterFrame->GetFrame());
  this->XImaginaryEntry = vtkKWEntry::New();
  this->XImaginaryEntry->SetParent(this->CenterFrame->GetFrame());
  
  this->CSpacingLabel = vtkKWLabel::New();
  this->CSpacingLabel->SetParent(this->Properties);
  this->CSpacingEntry = vtkKWEntry::New();
  this->CSpacingEntry->SetParent(this->Properties);
  
  this->XSpacingLabel = vtkKWLabel::New();
  this->XSpacingLabel->SetParent(this->Properties);
  this->XSpacingEntry = vtkKWEntry::New();
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
  this->XDimLabel->Delete();
  this->XDimLabel = NULL;
  this->XDimension->Delete();
  this->XDimension = NULL;
  this->YDimLabel->Delete();
  this->YDimLabel = NULL;
  this->YDimension->Delete();
  this->YDimension = NULL;
  this->ZDimLabel->Delete();
  this->ZDimLabel = NULL;
  this->ZDimension->Delete();
  this->ZDimension = NULL;
  
  this->CenterFrame->Delete();
  this->CRealLabel->Delete();
  this->CRealEntry->Delete();
  this->CImaginaryLabel->Delete();
  this->CImaginaryEntry->Delete();
  this->XRealLabel->Delete();
  this->XRealEntry->Delete();
  this->XImaginaryLabel->Delete();
  this->XImaginaryEntry->Delete();
  
  this->CSpacingLabel->Delete();
  this->CSpacingEntry->Delete();  

  this->XSpacingLabel->Delete();
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
  
  this->XDimLabel->Create(this->Application, "");
  this->XDimLabel->SetLabel("X:");
  this->XDimension->Create(this->Application, "-width 4");
  this->YDimLabel->Create(this->Application, "");
  this->YDimLabel->SetLabel("Y:");
  this->YDimension->Create(this->Application, "-width 4");
  this->ZDimLabel->Create(this->Application, "");
  this->ZDimLabel->SetLabel("Z:");
  this->ZDimension->Create(this->Application, "-width 4");
  this->Script("pack %s %s %s %s %s %s -side left", 
	       this->XDimLabel->GetWidgetName(),
	       this->XDimension->GetWidgetName(),
	       this->YDimLabel->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZDimLabel->GetWidgetName(),
	       this->ZDimension->GetWidgetName());

  this->CenterFrame->Create(this->Application);
  this->CenterFrame->SetLabel("Center Parameters");
  this->Script("pack %s", this->CenterFrame->GetWidgetName());
  
  this->CRealLabel->Create(this->Application, "");
  this->CRealLabel->SetLabel("C: ");
  this->CRealEntry->Create(this->Application, "-width 7");
  this->CRealEntry->SetValue(0.0);
  this->CImaginaryLabel->Create(this->Application, "");
  this->CImaginaryLabel->SetLabel(" + i");
  this->CImaginaryEntry->Create(this->Application, "-width 7");
  this->CImaginaryEntry->SetValue(0.0);  
  this->XRealLabel->Create(this->Application, "");
  this->XRealLabel->SetLabel("X: ");
  this->XRealEntry->Create(this->Application, "-width 7");
  this->XRealEntry->SetValue(0.0);
  this->XImaginaryLabel->Create(this->Application, "");
  this->XImaginaryLabel->SetLabel(" + i");
  this->XImaginaryEntry->Create(this->Application, "-width 7");
  this->XImaginaryEntry->SetValue(0.0);
  
  this->Script("pack %s %s %s %s %s %s %s %s -side left", 
	       this->CRealLabel->GetWidgetName(),
	       this->CRealEntry->GetWidgetName(),
	       this->CImaginaryLabel->GetWidgetName(),
	       this->CImaginaryEntry->GetWidgetName(),
	       this->XRealLabel->GetWidgetName(),
	       this->XRealEntry->GetWidgetName(),
	       this->XImaginaryLabel->GetWidgetName(),
	       this->XImaginaryEntry->GetWidgetName());
  
  
  this->CSpacingLabel->Create(this->Application, "");
  this->CSpacingLabel->SetLabel("C Spacing: ");
  this->CSpacingEntry->Create(this->Application, "-width 12");
  this->CSpacingEntry->SetValue(0.1);
  
  this->XSpacingLabel->Create(this->Application, "");
  this->XSpacingLabel->SetLabel("X Spacing: ");
  this->XSpacingEntry->Create(this->Application, "-width 12");
  this->XSpacingEntry->SetValue(0.1);
  
  this->Script("pack %s %s %s %s", 
	       this->CSpacingLabel->GetWidgetName(),
	       this->CSpacingEntry->GetWidgetName(),
	       this->XSpacingLabel->GetWidgetName(),
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
    this->InitializeData();
    }
  
  window->GetMainView()->SetSelectedComposite(this);
  this->GetView()->Render();
  window->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
vtkImageMandelbrotSource *vtkPVImageMandelbrotSource::GetImageMandelbrotSource()
{
  return vtkImageMandelbrotSource::SafeDownCast(this->ImageSource);  
}
