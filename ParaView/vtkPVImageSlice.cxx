/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSlice.cxx
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

#include "vtkPVImageSlice.h"
#include "vtkPVApplication.h"
#include "vtkPVRenderView.h"
#include "vtkPVImage.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkPVAssignment.h"
#include "vtkInteractorStyleImageExtent.h"
#include "vtkKWToolbar.h"

int vtkPVImageSliceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageSlice::vtkPVImageSlice()
{
  this->CommandFunction = vtkPVImageSliceCommand;
  
  this->Accept = vtkKWPushButton::New();
  this->Accept->SetParent(this->Properties);
  this->SourceButton = vtkKWPushButton::New();
  this->SourceButton->SetParent(this->Properties);
  
  this->SliceEntry = vtkKWLabeledEntry::New();
  this->SliceEntry->SetParent(this->Properties);
  this->XDimension = vtkKWRadioButton::New();
  this->XDimension->SetParent(this->Properties);
  this->YDimension = vtkKWRadioButton::New();
  this->YDimension->SetParent(this->Properties);
  this->ZDimension = vtkKWRadioButton::New();
  this->ZDimension->SetParent(this->Properties);
  
  this->Slice = vtkImageClip::New();
  this->Slice->ClipDataOn();
  
  this->PropertiesCreated = 0;
  this->SliceNumber = 0;
  this->SliceAxis = 3;
  
  this->SliceStyle = vtkInteractorStyleImageExtent::New();
  this->SliceStyle->ConstrainSpheresOn();
  this->SliceStyleButton = vtkKWPushButton::New();
  this->SliceStyleCreated = 0;
}

//----------------------------------------------------------------------------
vtkPVImageSlice::~vtkPVImageSlice()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  this->SourceButton->Delete();
  this->SourceButton = NULL;
  
  this->SliceEntry->Delete();
  this->SliceEntry = NULL;
  this->XDimension->Delete();
  this->XDimension = NULL;
  this->YDimension->Delete();
  this->YDimension = NULL;
  this->ZDimension->Delete();
  this->ZDimension = NULL;
  
  this->Slice->Delete();
  this->Slice = NULL;
  
  this->SliceStyle->Delete();
  this->SliceStyle = NULL;
  this->SliceStyleButton->Delete();
  this->SliceStyleButton = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageSlice* vtkPVImageSlice::New()
{
  return new vtkPVImageSlice();
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::CreateProperties()
{
  if (this->PropertiesCreated)
    {
    vtkErrorMacro("Properties already created.");
    return;
    }
  this->PropertiesCreated = 1;
 
  this->GetSliceStyle()->
    SetImageData((vtkImageData*)this->GetInput()->GetData());
  this->GetSliceStyle()->
    SetExtent(this->GetSlice()->GetOutputWholeExtent());
  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
  this->XDimension->Create(this->Application, "-text X");
  this->XDimension->SetCommand(this, "SelectXCallback");
  this->YDimension->Create(this->Application, "-text Y");
  this->YDimension->SetCommand(this, "SelectYCallback");
  this->ZDimension->Create(this->Application, "-text Z");
  this->ZDimension->SetCommand(this, "SelectZCallback");

  this->SliceEntry->Create(this->Application);
  this->SliceEntry->SetLabel("Slice:");
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "SliceChanged");
  this->Script("pack %s %s %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->SliceEntry->GetWidgetName(),
	       this->XDimension->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZDimension->GetWidgetName());

  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SelectXCallback()
{
  int ext[6];
  ((vtkImageData*)this->GetInput()->GetData())->GetWholeExtent(ext);
  int sliceNum = this->GetSliceEntry()->GetValueAsInt();
  
  this->YDimension->SetState(0);
  this->ZDimension->SetState(0);
  this->GetSliceStyle()->SetConstraint0ToCollapse();
  this->GetSliceStyle()->SetConstraint1ToNone();
  this->GetSliceStyle()->SetConstraint2ToNone();
  this->GetSliceStyle()->SetExtent(sliceNum, sliceNum, ext[2], ext[3], ext[4],
				   ext[5]);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SelectYCallback()
{
  int ext[6];
  ((vtkImageData*)this->GetInput()->GetData())->GetWholeExtent(ext);
  int sliceNum = this->GetSliceEntry()->GetValueAsInt();
  
  this->XDimension->SetState(0);
  this->ZDimension->SetState(0);
  this->GetSliceStyle()->SetConstraint0ToNone();
  this->GetSliceStyle()->SetConstraint1ToCollapse();
  this->GetSliceStyle()->SetConstraint2ToNone();
  this->GetSliceStyle()->SetExtent(ext[0], ext[1], sliceNum, sliceNum, ext[4],
				   ext[5]);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SelectZCallback()
{
  int ext[6];
  ((vtkImageData*)this->GetInput()->GetData())->GetWholeExtent(ext);
  int sliceNum = this->GetSliceEntry()->GetValueAsInt();
  
  this->XDimension->SetState(0);
  this->YDimension->SetState(0);
  this->GetSliceStyle()->SetConstraint0ToNone();
  this->GetSliceStyle()->SetConstraint1ToNone();
  this->GetSliceStyle()->SetConstraint2ToCollapse();
  this->GetSliceStyle()->SetExtent(ext[0], ext[1], ext[2], ext[3], sliceNum,
				   sliceNum);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::UpdateProperties()
{
  if ( ! this->PropertiesCreated)
    {
    return;
    }
  
  this->SliceEntry->SetValue(this->SliceNumber);

  this->XDimension->SetState(0);
  this->YDimension->SetState(0);
  this->ZDimension->SetState(0);
  if (this->SliceAxis == 0)
    {
    this->XDimension->SetState(1);
    this->GetSliceStyle()->SetConstraint0ToCollapse();
    }
  if (this->SliceAxis == 1)
    {
    this->YDimension->SetState(1);
    this->GetSliceStyle()->SetConstraint1ToCollapse();
    }
  if (this->SliceAxis == 2)
    {
    this->ZDimension->SetState(1);
    this->GetSliceStyle()->SetConstraint2ToCollapse();
    }
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetSliceNumber(int num)
{
  if (this->SliceNumber == num)
    {
    return;
    }
  
  this->Modified();
  this->SliceNumber = num;
  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetSliceAxis(int axis)
{
  if (this->SliceAxis == axis)
    {
    return;
    }
  
  this->Modified();
  this->SliceAxis = axis;
  this->UpdateProperties();
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetOutputWholeExtent(int xmin, int xmax, int ymin, 
					   int ymax, int zmin, int zmax)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->Slice->SetOutputWholeExtent(xmin, xmax, ymin, ymax, zmin, zmax);
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutputWholeExtent %d %d %d %d %d %d",
		   this->GetTclName(), xmin,xmax, ymin,ymax, zmin,zmax);
    }
}

//----------------------------------------------------------------------------
// Functions to update the progress bar
void StartImageSliceProgress(void *arg)
{
  vtkPVImageSlice *me = (vtkPVImageSlice*)arg;
  me->GetWindow()->SetStatusText("Processing ImageSlice");
}

//----------------------------------------------------------------------------
void ImageSliceProgress(void *arg)
{
  vtkPVImageSlice *me = (vtkPVImageSlice*)arg;
  me->GetWindow()->GetProgressGauge()->SetValue((int)(me->GetSlice()->GetProgress() * 100));
}

//----------------------------------------------------------------------------
void EndImageSliceProgress(void *arg)
{
  vtkPVImageSlice *me = (vtkPVImageSlice*)arg;
  me->GetWindow()->SetStatusText("");
  me->GetWindow()->GetProgressGauge()->SetValue(0);
}

//----------------------------------------------------------------------------
void SliceCallback(void *arg)
{
  vtkPVImageSlice *me = (vtkPVImageSlice*)arg;
  int *extent;
  
  me->GetSliceStyle()->DefaultCallback(me->GetSliceStyle()->GetCallbackType());
  
  extent = me->GetSliceStyle()->GetExtent();
  if (me->GetSliceStyle()->GetConstraint0())
    {
    me->GetSliceEntry()->SetValue(extent[0]);
    }
  else if (me->GetSliceStyle()->GetConstraint1())
    {
    me->GetSliceEntry()->SetValue(extent[2]);
    }
  else
    {
    me->GetSliceEntry()->SetValue(extent[4]);
    }
  
  me->SliceChanged();
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::UseSliceStyle()
{
  this->GetWindow()->GetMainView()->SetInteractorStyle(this->SliceStyle);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SliceChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVImage *pvi;
  vtkPVWindow *window = this->GetWindow();
  vtkPVActorComposite *ac;
  vtkPVAssignment *a;
  int *ext;
  
  // Get the parameter values from the properties UI.
  if (this->XDimension->GetState())
    {
    this->SliceAxis = 0;
    }
  else if (this->YDimension->GetState())
    {
    this->SliceAxis = 1;
    }
  else
    {
    this->SliceAxis = 2;
    }
  this->SliceNumber = this->SliceEntry->GetValueAsInt();
  
  // Set the extent of the clip filter.
  this->Slice->GetInput()->UpdateInformation();
  ext = this->Slice->GetInput()->GetWholeExtent();
  
  if (this->SliceAxis == 0)
    {
    if (this->SliceNumber < ext[0])
      {
      this->SetSliceNumber(ext[0]);
      }
    if (this->SliceNumber > ext[1])
      {
      this->SetSliceNumber(ext[1]);
      }
    this->SetOutputWholeExtent(this->SliceNumber, this->SliceNumber,
			       ext[2], ext[3], ext[4], ext[5]);
    }
  if (this->SliceAxis == 1)
    {
    if (this->SliceNumber < ext[2])
      {
      this->SetSliceNumber(ext[2]);
      }
    if (this->SliceNumber > ext[3])
      {
      this->SetSliceNumber(ext[3]);
      }
    this->SetOutputWholeExtent(ext[0], ext[1], this->SliceNumber, 
			       this->SliceNumber, ext[4], ext[5]);
    }
  if (this->SliceAxis == 2)
    {
    if (this->SliceNumber < ext[4])
      {
      this->SetSliceNumber(ext[4]);
      }
    if (this->SliceNumber > ext[5])
      {
      this->SetSliceNumber(ext[5]);
      }
    this->SetOutputWholeExtent(ext[0], ext[1], ext[2], ext[3],
			       this->SliceNumber, this->SliceNumber);
    }
  
  // Create the data if this is the first accept.
  if (this->GetPVData() == NULL)
    {
    this->GetSlice()->SetStartMethod(StartImageSliceProgress, this);
    this->GetSlice()->SetProgressMethod(ImageSliceProgress, this);
    this->GetSlice()->SetEndMethod(EndImageSliceProgress, this);
    this->GetSliceStyle()->SetCallbackMethod(SliceCallback, this);
    pvi = vtkPVImage::New();
    pvi->Clone(pvApp);
    pvi->OutlineFlagOff();
    this->SetOutput(pvi);
    a = this->GetInput()->GetAssignment();
    pvi->SetAssignment(a);  
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);

    this->GetSliceStyle()->SetExtent(this->GetSlice()->GetOutputWholeExtent());
    
    // Lets try to pick a reasonable scalar range.
    float range[2];
    // The GetScalarRange is in vtkPVActorComposite instead of vtkPVData,
    // because the actor composite knows what piece to request.
    ac->GetInputScalarRange(range);
    ac->SetScalarRange(range[0], range[1]);
    }

  window->GetMainView()->SetSelectedComposite(this);
  
  if (!this->SliceStyleCreated)
    {
    this->SliceStyleCreated = 1;
    this->Script("%s configure -state normal",
		 this->SliceStyleButton->GetWidgetName());
    }
  
  this->GetView()->Render();  
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetInput(vtkPVImage *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->Slice->SetInput(pvData->GetImageData());
  this->Input = pvData;
}


//----------------------------------------------------------------------------
void vtkPVImageSlice::SetOutput(vtkPVImage *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(),
			   pvi->GetTclName());
    }  
  
  this->SetPVData(pvi);
  pvi->SetData(this->Slice->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageSlice::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::Select(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Select(view);
  
  if (!this->SliceStyleCreated)
    {
    this->SliceStyleButton->SetParent(this->GetWindow()->GetToolbar());
    this->SliceStyleButton->Create(this->Application, "");
    this->SliceStyleButton->SetLabel("Slice");
    this->SliceStyleButton->SetCommand(this, "UseSliceStyle");
    this->Script("%s configure -state disabled",
		 this->SliceStyleButton->GetWidgetName());
    }
  this->Script("pack %s -side left -pady 0 -fill none -expand no",
	       this->SliceStyleButton->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::Deselect(vtkKWView *view)
{
  // invoke super
  this->vtkPVSource::Deselect(view);
  
  // unpack extent style button and reset interactor style to trackball camera
  this->Script("pack forget %s", this->SliceStyleButton->GetWidgetName());
  this->GetWindow()->UseCameraStyle();
}
