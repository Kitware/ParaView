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
#include "vtkPVAssignment.h"
#include "vtkInteractorStyleImageExtent.h"
#include "vtkKWToolbar.h"

int vtkPVImageSliceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageSlice::vtkPVImageSlice()
{
  this->CommandFunction = vtkPVImageSliceCommand;
  
  this->SliceNumber = 0;
  this->SliceAxis = 3;
  
  this->SliceStyle = vtkInteractorStyleImageExtent::New();
  this->SliceStyle->ConstrainSpheresOn();
  this->SliceStyleButton = vtkKWPushButton::New();
  this->SliceStyleCreated = 0;

  vtkImageClip *clip = vtkImageClip::New();
  clip->ClipDataOn();
  this->SetVTKSource(clip);
  clip->Delete();
}

//----------------------------------------------------------------------------
vtkPVImageSlice::~vtkPVImageSlice()
{ 
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
  
  this->AddScale("Slice:","SetSliceNumber","GetSliceNumber", 
                 0,100,1, this);
  this->AddModeList("Axis:", "SetSliceAxis","GetSliceAxis", this);
  this->AddModeListItem("X Axis", 0);
  this->AddModeListItem("Y Axis", 1);
  this->AddModeListItem("Z Axis", 2);
  
  this->UpdateProperties();
}


//----------------------------------------------------------------------------
void vtkPVImageSlice::UpdateProperties()
{
  this->UpdateParameterWidgets();
  this->UpdateNavigationCanvas();  
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
void vtkPVImageSlice::UseSliceStyle()
{
  this->GetWindow()->GetMainView()->SetInteractorStyle(this->SliceStyle);
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
