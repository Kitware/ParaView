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
  
  this->SliceEntry = vtkKWEntry::New();
  this->SliceEntry->SetParent(this->Properties);
  this->SliceLabel = vtkKWLabel::New();
  this->SliceLabel->SetParent(this->Properties);
  this->XDimension = vtkKWRadioButton::New();
  this->XDimension->SetParent(this->Properties);
  this->YDimension = vtkKWRadioButton::New();
  this->YDimension->SetParent(this->Properties);
  this->ZDimension = vtkKWRadioButton::New();
  this->ZDimension->SetParent(this->Properties);
  
  this->Slice = vtkImageClip::New();
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
void vtkPVImageSlice::CreateProperties()
{
  int *extents;
  int sliceNumber;
  
  // must set the application
  this->vtkPVSource::CreateProperties();
  
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
  
  this->SourceButton->Create(this->Application, "-text GetSource");
  this->SourceButton->SetCommand(this, "GetSource");
  this->Accept->Create(this->Application, "-text Accept");
  this->Accept->SetCommand(this, "SliceChanged");
  this->Script("pack %s %s %s %s %s %s %s",
	       this->SourceButton->GetWidgetName(),
	       this->Accept->GetWidgetName(),
	       this->SliceLabel->GetWidgetName(),
	       this->SliceEntry->GetWidgetName(),
	       this->XDimension->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZDimension->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::SliceChanged()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVImage *pvi;
  int newSliceNum = this->SliceEntry->GetValueAsInt();
  vtkPVWindow *window = this->GetWindow();
  vtkPVActorComposite *ac;
  vtkPVAssignment *a;
  
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
  
  if (this->GetPVData() == NULL)
    {
    pvi = vtkPVImage::New();
    pvi->Clone(pvApp);
    pvi->OutlineFlagOff();
    this->SetOutput(pvi);
    a = window->GetPreviousSource()->GetPVData()->GetAssignment();
    pvi->SetAssignment(a);  
    this->GetPVData()->GetData()->
      SetUpdateExtent(this->GetPVData()->GetData()->GetWholeExtent());
    this->GetInput()->GetActorComposite()->VisibilityOff();
    this->CreateDataPage();
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
    }
  
  this->Slice->Modified();
  this->Slice->Update();
  
  this->GetView()->Render();  
  window->GetMainView()->SetSelectedComposite(this);
  window->GetMainView()->ResetCamera();
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
void vtkPVImageSlice::SetInput(vtkPVImage *pvData)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvData->GetTclName());
    }  
  
  this->GetSlice()->SetInput(pvData->GetImageData());
  this->Input = pvData;
}


//----------------------------------------------------------------------------
void vtkPVImageSlice::SetOutput(vtkPVImage *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetInput %s", this->GetTclName(),
			   pvi->GetTclName());
    }  
  
  this->SetPVData(pvi);
  pvi->SetImageData(this->Slice->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageSlice::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVImageSlice::GetSource()
{
  this->GetPVData()->GetActorComposite()->VisibilityOff();
  this->GetWindow()->GetMainView()->
    SetSelectedComposite(this->GetInput()->GetPVSource());
  this->GetInput()->GetActorComposite()->VisibilityOn();
  this->GetView()->Render();
  this->GetWindow()->GetMainView()->ResetCamera();
}
