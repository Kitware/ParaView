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
#include "vtkKWApplication.h"
#include "vtkKWView.h"
#include "vtkKWRenderView.h"
#include "vtkPVComposite.h"
#include "vtkPVImage.h"

int vtkPVImageSliceCommand(ClientData cd, Tcl_Interp *interp,
			   int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageSlice::vtkPVImageSlice()
{
  this->CommandFunction = vtkPVImageSliceCommand;
  
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this);
  
  this->SliceEntry = vtkKWEntry::New();
  this->SliceEntry->SetParent(this);
  this->SliceLabel = vtkKWLabel::New();
  this->SliceLabel->SetParent(this);
  this->XDimension = vtkKWRadioButton::New();
  this->XDimension->SetParent(this);
  this->YDimension = vtkKWRadioButton::New();
  this->YDimension->SetParent(this);
  this->ZDimension = vtkKWRadioButton::New();
  this->ZDimension->SetParent(this);
  
  this->Slice = vtkImageClip::New();
}

//----------------------------------------------------------------------------
vtkPVImageSlice::~vtkPVImageSlice()
{ 
  this->Accept->Delete();
  this->Accept = NULL;
  
  this->SliceEntry->Delete();
  this->SliceEntry = NULL;
  this->SliceLabel->Delete();
  this->SliceLabel = NULL;
  this->XDimension->Delete();
  this->XDimension = NULL;
  this->YDimension->Delete();
  this->YDimension = NULL;
  this->ZDimension->Delete();
  this->ZDimension = NULL;
  
  this->Slice->Delete();
  this->Slice = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageSlice* vtkPVImageSlice::New()
{
  return new vtkPVImageSlice();
}

//----------------------------------------------------------------------------
int vtkPVImageSlice::Create(char *args)
{
  int *extents;
  int sliceNumber;
  
  // must set the application
  if (this->vtkPVSource::Create(args) == 0)
    {
    return 0;
    }
  
  this->XDimension->Create(this->Application, "-text X");
  this->XDimension->SetCommand(this, "SelectX");
  this->YDimension->Create(this->Application, "-text Y");
  this->YDimension->SetCommand(this, "SelectY");
  this->ZDimension->Create(this->Application, "-text Z");
  this->ZDimension->SetCommand(this, "SelectZ");
  
  extents = this->GetSlice()->GetOutputWholeExtent();
  if (extents[0] == extents[1])
    {
    sliceNumber = extents[0];
    this->SelectX();
    }
  else if (extents[2] == extents[3])
    {
    sliceNumber = extents[2];
    this->SelectY();
    }
  else
    {
    sliceNumber = extents[4];
    this->SelectZ();
    }
  
  this->SliceLabel->Create(this->Application, "");
  this->SliceLabel->SetLabel("Slice:");
  this->SliceEntry->Create(this->Application, "");
  this->SliceEntry->SetValue(sliceNumber);
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "SliceChanged");
  this->Script("pack %s %s %s %s %s %s",
	       this->Accept->GetWidgetName(),
	       this->SliceLabel->GetWidgetName(),
	       this->SliceEntry->GetWidgetName(),
	       this->XDimension->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZDimension->GetWidgetName());

  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SliceChanged()
{
  int newSliceNum = this->SliceEntry->GetValueAsInt();
  
  if (this->XDimension->GetState())
    {
    this->Slice->SetOutputWholeExtent(newSliceNum, newSliceNum,
				      this->Dimensions[2],
				      this->Dimensions[3],
				      this->Dimensions[4],
				      this->Dimensions[5]);
    }
  else if (this->YDimension->GetState())
    {
    this->Slice->SetOutputWholeExtent(this->Dimensions[0],
				      this->Dimensions[1],
				      newSliceNum, newSliceNum,
				      this->Dimensions[4],
				      this->Dimensions[5]);
    }
  else
    {
    this->Slice->SetOutputWholeExtent(this->Dimensions[0],
				      this->Dimensions[1],
				      this->Dimensions[2],
				      this->Dimensions[3],
				      newSliceNum, newSliceNum);
    }
  
  this->Slice->Modified();
  this->Slice->Update();
  
  this->Composite->GetView()->Render();
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SelectX()
{
  this->XDimension->SetState(1);
  this->YDimension->SetState(0);
  this->ZDimension->SetState(0);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SelectY()
{
  this->XDimension->SetState(0);
  this->YDimension->SetState(1);
  this->ZDimension->SetState(0);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SelectZ()
{
  this->XDimension->SetState(0);
  this->YDimension->SetState(0);
  this->ZDimension->SetState(1);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetDimensions(int dim[6])
{
  int i;
  
  for (i = 0; i < 6; i++)
    {
    this->Dimensions[i] = dim[i];
    }
}

//----------------------------------------------------------------------------
int* vtkPVImageSlice::GetDimensions()
{
  return this->Dimensions;
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SetOutput(vtkPVImage *pvi)
{
  this->SetPVData(pvi);

  pvi->SetImageData(this->Slice->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageSlice::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}









