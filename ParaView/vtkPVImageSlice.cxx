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
#include "vtkPVImageData.h"
#include "vtkPVWindow.h"
#include "vtkPVActorComposite.h"
#include "vtkInteractorStyleImageExtent.h"
#include "vtkKWToolbar.h"
#include "vtkKWScale.h"
#include "vtkPVSelectionList.h"

int vtkPVImageSliceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageSlice::vtkPVImageSlice()
{
  this->CommandFunction = vtkPVImageSliceCommand;
  
  this->SliceNumber = 0;
  this->SliceAxis = 2;
  
  this->SliceStyle = vtkInteractorStyleImageExtent::New();
  this->SliceStyle->ConstrainSpheresOn();
  this->SliceStyleButton = vtkKWPushButton::New();
  this->SliceStyleCreated = 0;

  vtkImageClip *clip = vtkImageClip::New();
  clip->ClipDataOn();
  this->SetVTKSource(clip);
  clip->Delete();

  this->AxisWidget = NULL;
  this->SliceScale = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageSlice::~vtkPVImageSlice()
{ 
  this->SliceStyle->Delete();
  this->SliceStyle = NULL;
  this->SliceStyleButton->Delete();
  this->SliceStyleButton = NULL;
  if (this->AxisWidget)
    {
    this->AxisWidget->UnRegister(this);
    this->AxisWidget = NULL;
    }
  if (this->SliceScale)
    {
    this->SliceScale->UnRegister(this);
    this->SliceScale = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVImageSlice* vtkPVImageSlice::New()
{
  return new vtkPVImageSlice();
}

//----------------------------------------------------------------------------
vtkImageClip* vtkPVImageSlice::GetImageClip()
{
  return vtkImageClip::SafeDownCast(this->GetVTKSource());
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::CreateProperties()
{
  // must set the application
  this->vtkPVImageToImageFilter::CreateProperties();
 
  this->GetSliceStyle()->SetImageData(this->GetImageClip()->GetInput());
  this->GetSliceStyle()->SetExtent(this->GetImageClip()->GetOutputWholeExtent());
  
  this->AxisWidget = this->AddModeList("Axis:", "SetSliceAxis","GetSliceAxis", this);
  this->AxisWidget->Register(this);
  this->AddModeListItem("X", 0);
  this->AddModeListItem("Y", 1);
  this->AddModeListItem("Z", 2);
  
  this->SliceScale = this->AddScale("Slice:","SetSliceNumber","GetSliceNumber", 
                                    0,100,1, this);
  this->SliceScale->Register(this);

  this->UpdateParameterWidgets();
  this->ComputeSliceRange();
  this->AxisWidget->SetCommand(this, "ComputeSliceRange");
}


//----------------------------------------------------------------------------
void vtkPVImageSlice::UpdateVTKSource()
{
  vtkImageClip *clip = this->GetImageClip();
  int ext[6];

  clip->GetInput()->GetWholeExtent(ext);
  if (this->SliceAxis == 0)
    {
    if (this->SliceNumber < ext[0])
      {
      this->SliceNumber = ext[0];
      }
    if (this->SliceNumber > ext[1])
      {
      this->SliceNumber = ext[1];
      }
    ext[0] = ext[1] = this->SliceNumber;
    }
  if (this->SliceAxis == 1)
    {
    if (this->SliceNumber < ext[2])
      {
      this->SliceNumber = ext[2];
      }
    if (this->SliceNumber > ext[3])
      {
      this->SliceNumber = ext[3];
      }
    ext[2] = ext[3] = this->SliceNumber;
    }
  if (this->SliceAxis == 2)
    {
    if (this->SliceNumber < ext[4])
      {
      this->SliceNumber = ext[4];
      }
    if (this->SliceNumber > ext[5])
      {
      this->SliceNumber = ext[5];
      }
    ext[4] = ext[5] = this->SliceNumber;
    }

  vtkDebugMacro("slice: " << this->SliceNumber << ", axes: " << this->SliceAxis);
  vtkDebugMacro("ext: " << ext[0] << ", " << ext[1] << ", " << ext[2]
                << ", " << ext[3] << ", " << ext[4] << ", " << ext[5]);

  clip->SetOutputWholeExtent(ext);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetSliceNumber(int num)
{
  if (this->SliceNumber == num)
    { // This is needed in case the user chooses the default values.
    this->UpdateVTKSource();
    return;
    }
  
  this->Modified();
  this->SliceNumber = num;
  this->UpdateVTKSource();
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
  this->UpdateVTKSource();
}


//----------------------------------------------------------------------------
void vtkPVImageSlice::UseSliceStyle()
{
  this->GetWindow()->GetMainView()->SetInteractorStyle(this->SliceStyle);
}


//----------------------------------------------------------------------------
void vtkPVImageSlice::ComputeSliceRange()
{
  int *ext;
  int axis, min, max;

  ext = this->GetImageClip()->GetInput()->GetWholeExtent();
  axis = this->AxisWidget->GetCurrentValue();
  if (axis == 0)
    {
    min = ext[0];
    max = ext[1];
    }
  if (axis == 1)
    {
    min = ext[2];
    max = ext[3];
    }
  if (axis == 2)
    {
    min = ext[4];
    max = ext[5];
    }

  this->SliceScale->SetRange(min, max);
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
