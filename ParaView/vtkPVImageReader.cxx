/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageReader.cxx
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

#include "vtkPVImageReader.h"
#include "vtkPVApplication.h"
#include "vtkPVImage.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

//----------------------------------------------------------------------------
vtkPVImageReader::vtkPVImageReader()
{
  this->Accept = vtkKWWidget::New();
  this->Accept->SetParent(this->Properties);
  this->Open = vtkKWWidget::New();
  this->Open->SetParent(this->Properties);
  
  this->XLabel = vtkKWLabel::New();
  this->XLabel->SetParent(this->Properties);
  this->XDimension = vtkKWEntry::New();
  this->XDimension->SetParent(this->Properties);
  this->YLabel = vtkKWLabel::New();
  this->YLabel->SetParent(this->Properties);
  this->YDimension = vtkKWEntry::New();
  this->YDimension->SetParent(this->Properties);
  this->ZLabel = vtkKWLabel::New();
  this->ZLabel->SetParent(this->Properties);
  this->ZDimension = vtkKWEntry::New();
  this->ZDimension->SetParent(this->Properties);
  
  this->ImageReader = vtkImageReader::New();
  this->ImageReader->SetFilePrefix("../../vtkdata/headsq/quarter");
}

//----------------------------------------------------------------------------
vtkPVImageReader::~vtkPVImageReader()
{
  this->Accept->Delete();
  this->Accept = NULL;
  this->Open->Delete();
  this->Open = NULL;
  
  this->ImageReader->Delete();
  this->ImageReader = NULL;
}

//----------------------------------------------------------------------------
vtkPVImageReader* vtkPVImageReader::New()
{
  return new vtkPVImageReader();
}

//----------------------------------------------------------------------------
void vtkPVImageReader::CreateProperties()
{  
  this->vtkPVSource::CreateProperties();
  
  this->Accept->Create(this->Application, "button", "-text Accept");
  this->Accept->SetCommand(this, "ImageAccepted");
  this->Script("pack %s", this->Accept->GetWidgetName());
  
  this->Open->Create(this->Application, "button", "-text OpenFile");
  this->Open->SetCommand(this, "OpenFile");
  this->Script("pack %s", this->Open->GetWidgetName());
  
  this->XLabel->Create(this->Application, "");
  this->XLabel->SetLabel("X Dim.");
  this->XDimension->Create(this->Application, "");
  this->XDimension->SetValue(63);
  this->YLabel->Create(this->Application, "");
  this->YLabel->SetLabel("Y Dim.");
  this->YDimension->Create(this->Application, "");
  this->YDimension->SetValue(63);
  this->ZLabel->Create(this->Application, "");
  this->ZLabel->SetLabel("Z Dim.");
  this->ZDimension->Create(this->Application, "");
  this->ZDimension->SetValue(93);
  this->Script("pack %s %s %s %s %s %s", this->XLabel->GetWidgetName(),
	       this->XDimension->GetWidgetName(),
	       this->YLabel->GetWidgetName(),
	       this->YDimension->GetWidgetName(),
	       this->ZLabel->GetWidgetName(),
	       this->ZDimension->GetWidgetName());
}

//----------------------------------------------------------------------------
void vtkPVImageReader::SetOutput(vtkPVImage *pvi)
{
  this->SetPVData(pvi);

  pvi->SetImageData(this->ImageReader->GetOutput());
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageReader::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVImageReader::ReadImage()
{
  this->ImageReader->SetDataByteOrderToLittleEndian();
  this->ImageReader->SetDataExtent(0, this->XDimension->GetValueAsInt(),
				   0, this->YDimension->GetValueAsInt(),
				   1, this->ZDimension->GetValueAsInt());
  this->ImageReader->SetDataSpacing(4, 4, 1.8);
}

//----------------------------------------------------------------------------
void vtkPVImageReader::ImageAccepted()
{
  vtkPVApplication *pvApp = vtkPVApplication::SafeDownCast(this->Application);
  vtkPVImage *image;
  vtkPVAssignment *a;
  vtkPVActorComposite *ac;
  vtkPVWindow *window = this->GetWindow();
  
  image = vtkPVImage::New();
  image->Clone(pvApp);
  a = vtkPVAssignment::New();
  a->Clone(pvApp);
  a->SetOriginalImage(image);

  // Does not actually read.  Just sets the file name ...
  this->ReadImage();
  
  this->SetOutput(image);
  image->SetAssignment(a);
  
  this->GetView()->Render();
  
  this->CreateDataPage();
  
  ac = this->GetPVData()->GetActorComposite();
  window->GetMainView()->AddComposite(ac);
  window->GetMainView()->SetSelectedComposite(this);
}

//----------------------------------------------------------------------------
void vtkPVImageReader::OpenFile()
{
  // We need to figure out what to do if the image is stored in multiple files
  // (so we only need a file prefix, not a file).
  this->Script("tk_getOpenFile -title \"Open Image File\"");
}
