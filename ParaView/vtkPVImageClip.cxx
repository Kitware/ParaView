/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageClip.cxx
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
#include "vtkPVImageClip.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVImageData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkPVScalarBar.h"
#include "vtkPVAssignment.h"
#include "vtkKWToolbar.h"

int vtkPVImageClipCommand(ClientData cd, Tcl_Interp *interp,
			  int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageClip::vtkPVImageClip()
{
  this->CommandFunction = vtkPVImageClipCommand;
  
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  
  this->ClipXMinEntry = vtkKWLabeledEntry::New();
  this->ClipXMinEntry->SetParent(this->Properties);
  this->ClipXMaxEntry = vtkKWLabeledEntry::New();
  this->ClipXMaxEntry->SetParent(this->Properties);
  this->ClipYMinEntry = vtkKWLabeledEntry::New();
  this->ClipYMinEntry->SetParent(this->Properties);
  this->ClipYMaxEntry = vtkKWLabeledEntry::New();
  this->ClipYMaxEntry->SetParent(this->Properties);
  this->ClipZMinEntry = vtkKWLabeledEntry::New();
  this->ClipZMinEntry->SetParent(this->Properties);
  this->ClipZMaxEntry = vtkKWLabeledEntry::New();
  this->ClipZMaxEntry->SetParent(this->Properties);
  
  this->ExtentStyleButton = vtkKWPushButton::New();
  
  this->ImageClip = vtkImageClip::New();
  this->ImageClip->ClipDataOn();
  
  this->ExtentStyle = vtkInteractorStyleImageExtent::New();
  this->ExtentStyleCreated = 0;
}

//----------------------------------------------------------------------------
vtkPVImageClip::~vtkPVImageClip()
{
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->ClipXMinEntry->Delete();
  this->ClipXMinEntry = NULL;
  this->ClipXMaxEntry->Delete();
  this->ClipXMaxEntry = NULL;
  this->ClipYMinEntry->Delete();
  this->ClipYMinEntry = NULL;
  this->ClipYMaxEntry->Delete();
  this->ClipYMaxEntry = NULL;
  this->ClipZMinEntry->Delete();
  this->ClipZMinEntry = NULL;
  this->ClipZMaxEntry->Delete();
  this->ClipZMaxEntry = NULL;
  
  this->ExtentStyleButton->Delete();
  this->ExtentStyleButton = NULL;
  
  this->ImageClip->Delete();
  this->ImageClip = NULL;
  
  this->ExtentStyle->Delete();
  this->ExtentStyle = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageClip* vtkPVImageClip::New()
{
  return new vtkPVImageClip();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::CreateProperties()
{
  int *extents;
  
  // must set the application
  this->vtkPVSource::CreateProperties();

  extents = this->GetImageClip()->GetOutputWholeExtent();

  this->GetExtentStyle()->
    SetImageData((vtkImageData*)this->GetPVInput()->GetData());
  this->GetExtentStyle()->SetExtent(extents);
  
  this->ClipXMinEntry->Create(this->Application);
  this->ClipXMinEntry->SetLabel("X Min.:");
  this->ClipXMinEntry->SetValue(extents[0]);
  this->ClipXMaxEntry->Create(this->Application);
  this->ClipXMaxEntry->SetLabel("X Max.:");
  this->ClipXMaxEntry->SetValue(extents[1]);
  this->ClipYMinEntry->Create(this->Application);
  this->ClipYMinEntry->SetLabel("Y Min.:");
  this->ClipYMinEntry->SetValue(extents[2]);
  this->ClipYMaxEntry->Create(this->Application);
  this->ClipYMaxEntry->SetLabel("Y Max.:");
  this->ClipYMaxEntry->SetValue(extents[3]);
  this->ClipZMinEntry->Create(this->Application);
  this->ClipZMinEntry->SetLabel("Z Min.:");
  this->ClipZMinEntry->SetValue(extents[4]);
  this->ClipZMaxEntry->Create(this->Application);
  this->ClipZMaxEntry->SetLabel("Z Max.:");
  this->ClipZMaxEntry->SetValue(extents[5]);
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "ExtentsChanged");
  this->Script("pack %s %s %s %s %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->ClipXMinEntry->GetWidgetName(),
	       this->ClipXMaxEntry->GetWidgetName(),
	       this->ClipYMinEntry->GetWidgetName(),
	       this->ClipYMaxEntry->GetWidgetName(),
	       this->ClipZMinEntry->GetWidgetName(),
	       this->ClipZMaxEntry->GetWidgetName());
}


//----------------------------------------------------------------------------
void vtkPVImageClip::SetPVInput(vtkPVImageData *pvi)
{
  this->GetImageClip()->SetInput(pvi->GetImageData());
  this->vtkPVSource::SetNthPVInput(0, pvi);
  if (pvi)
    {
    this->PVInputs[0]->AddPVSourceToUsers(this);
    }
}

//----------------------------------------------------------------------------
void vtkPVImageClip::SetPVOutput(vtkPVImageData *pvi)
{
  this->vtkPVSource::SetPVOutput(pvi);

  pvi->SetData(this->ImageClip->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVImageData *vtkPVImageClip::GetPVOutput()
{
  return vtkPVImageData::SafeDownCast(this->PVOutput);
}

//----------------------------------------------------------------------------
void vtkPVImageClip::SetOutputWholeExtent(int xMin, int xMax, 
					  int yMin, int yMax, 
					  int zMin, int zMax)
{  
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutputWholeExtent %d %d %d %d %d %d", 
			   this->GetTclName(), xMin,xMax, yMin,yMax, zMin,zMax);
    }  

  this->ImageClip->SetOutputWholeExtent(xMin, xMax, yMin, yMax, zMin, zMax);
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartImageClipProgress(void *arg)
{
  vtkPVImageClip *me = (vtkPVImageClip*)arg;
  me->GetWindow()->SetStatusText("Processing ImageClip");
}

//----------------------------------------------------------------------------
void ImageClipProgress(void *arg)
{
  vtkPVImageClip *me = (vtkPVImageClip*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetImageClip()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndImageClipProgress(void *arg)
{
  vtkPVImageClip *me = (vtkPVImageClip*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void ExtentCallback(void *arg)
{
  vtkPVImageClip *me = (vtkPVImageClip*)arg;
  int *extent = new int[6];

  me->GetExtentStyle()->DefaultCallback(me->GetExtentStyle()->GetCallbackType());
  
  extent = me->GetExtentStyle()->GetExtent();
  me->GetClipXMinEntry()->SetValue(extent[0]);
  me->GetClipXMaxEntry()->SetValue(extent[1]);
  me->GetClipYMinEntry()->SetValue(extent[2]);
  me->GetClipYMaxEntry()->SetValue(extent[3]);
  me->GetClipZMinEntry()->SetValue(extent[4]);
  me->GetClipZMaxEntry()->SetValue(extent[5]);
  
  me->ExtentsChanged();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::UseExtentStyle()
{
  this->GetWindow()->GetMainView()->SetInteractorStyle(this->ExtentStyle);
}

//----------------------------------------------------------------------------
void vtkPVImageClip::ExtentsChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVImageData *pvi;
  vtkPVActorComposite *ac;
  vtkPVWindow *window = this->GetWindow();
  vtkPVAssignment *a;
  
  this->ImageClip->SetOutputWholeExtent(this->ClipXMinEntry->GetValueAsInt(),
					this->ClipXMaxEntry->GetValueAsInt(),
					this->ClipYMinEntry->GetValueAsInt(),
					this->ClipYMaxEntry->GetValueAsInt(),
					this->ClipZMinEntry->GetValueAsInt(),
					this->ClipZMaxEntry->GetValueAsInt());
  
  if (this->GetPVOutput() == NULL)
    {
    this->GetImageClip()->SetStartMethod(StartImageClipProgress, this);
    this->GetImageClip()->SetProgressMethod(ImageClipProgress, this);
    this->GetImageClip()->SetEndMethod(EndImageClipProgress, this);
    this->GetExtentStyle()->SetCallbackMethod(ExtentCallback, this);
    pvi = vtkPVImageData::New();
    pvi->Clone(pvApp);
    this->SetPVOutput(pvi);
    a = window->GetPreviousSource()->GetPVOutput()->GetAssignment();
    pvi->SetAssignment(a);
    this->GetPVOutput()->GetData()->
      SetUpdateExtent(this->GetPVOutput()->GetData()->GetWholeExtent());
    this->GetPVInput()->GetActorComposite()->VisibilityOff();
    this->GetPVInput()->GetScalarBar()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVOutput()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    window->GetSourceList()->Update();
    }
  window->GetMainView()->SetSelectedComposite(this);

  if (!this->ExtentStyleCreated)
    {
    this->ExtentStyleCreated = 1;
    this->Script("%s configure -state normal",
		 this->ExtentStyleButton->GetWidgetName());
    }

  this->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::GetSource()
{
  this->GetPVOutput()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetPVInput()->GetPVSource());
  this->GetPVInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::Deselect(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Deselect(view);
  
  // unpack extent style button and reset interactor style to trackball camera
  this->Script("pack forget %s", this->ExtentStyleButton->GetWidgetName());
  this->GetWindow()->UseCameraStyle();
}

//----------------------------------------------------------------------------
void vtkPVImageClip::Select(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Select(view);
  
  if (!this->ExtentStyleCreated)
    {
    this->ExtentStyleButton->SetParent(this->GetWindow()->GetToolbar());
    this->ExtentStyleButton->Create(this->Application, "");
    this->ExtentStyleButton->SetLabel("Image Extent");
    this->ExtentStyleButton->SetCommand(this, "UseExtentStyle");
    this->Script("%s configure -state disabled",
		 this->ExtentStyleButton->GetWidgetName());
    }
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
	       this->ExtentStyleButton->GetWidgetName());
}

//---------------------------------------------------------------------------
vtkPVImageData *vtkPVImageClip::GetPVInput()
{
  return (vtkPVImageData *)(this->vtkPVSource::GetNthPVInput(0));
}
