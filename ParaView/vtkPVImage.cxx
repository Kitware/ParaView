/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImage.cxx
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
#include "vtkPVImage.h"
#include "vtkPVImageClip.h"
#include "vtkPVImageSlice.h"
#include "vtkPVWindow.h"
#include "vtkImageOutlineFilter.h"
#include "vtkPVAssignment.h"
#include "vtkGeometryFilter.h"

int vtkPVImageCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImage::vtkPVImage()
{
  this->OutlineFlag = 1;
  this->CommandFunction = vtkPVImageCommand;
}

//----------------------------------------------------------------------------
vtkPVImage::~vtkPVImage()
{
}

//----------------------------------------------------------------------------
vtkPVImage* vtkPVImage::New()
{
  return new vtkPVImage();
}

//----------------------------------------------------------------------------
void vtkPVImage::Clip()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVImageClip *clip;
  int ext[6];

  clip = vtkPVImageClip::New();
  clip->Clone(pvApp);
  
  this->GetImageData()->GetWholeExtent(ext);
  ext[5] = (ext[4] + ext[5])/2;
  ext[4] = ext[5];
  
  clip->SetInput(this);
  clip->SetOutputWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
  
  clip->SetName("clip");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  this->GetPVSource()->GetView()->AddComposite(clip);
  
  window->SetCurrentSource(clip);
  window->GetSourceList()->Update();
  
  clip->Delete();
}

//----------------------------------------------------------------------------
void vtkPVImage::Slice()
{
  vtkPVApplication *pvApp = (vtkPVApplication *)this->Application;
  vtkPVImageSlice *slice;
  int *extents;
  
  slice = vtkPVImageSlice::New();
  slice->Clone(pvApp);

  slice->SetInput(this);
  
  extents = this->GetImageData()->GetExtent();
  slice->SetDimensions(extents);
  
  extents[5] = (extents[4] + extents[5])/2;
  extents[4] = extents[5];
  
  slice->GetSlice()->ClipDataOn();
  slice->GetSlice()->SetOutputWholeExtent(extents);
  slice->GetSlice()->Update();

  slice->SetName("slice");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  this->GetPVSource()->GetView()->AddComposite(slice);
  
  window->SetCurrentSource(slice);
  window->GetSourceList()->Update();
  
  slice->Delete();
}

//----------------------------------------------------------------------------
int vtkPVImage::Create(char *args)
{
  if (this->vtkPVData::Create(args) == 0)
    {
    return 0;
    }
  
  this->FiltersMenuButton->AddCommand("vtkImageClip", this, "Clip");
  this->FiltersMenuButton->AddCommand("ImageSlice", this, "Slice");
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImage::SetImageData(vtkImageData *image)
{
  vtkImageOutlineFilter *outline;
  vtkGeometryFilter *geometry;

  this->SetData(image);
  this->ActorComposite->SetApplication(this->Application);
  
  // This should really be changed to switch mappers.  The flag
  // could them be turned on and off ...
  if (this->OutlineFlag)
    {
    image->UpdateInformation();
    outline = vtkImageOutlineFilter::New();
    outline->SetInput(image);
    this->Mapper->SetInput(outline->GetOutput());
    this->ActorComposite->SetInput(outline->GetOutput());
    outline->Delete();
    }
  else
    {
    image->UpdateInformation();
    geometry = vtkGeometryFilter::New();
    geometry->SetInput(image);
    this->Mapper->SetInput(geometry->GetOutput());
    this->Mapper->SetScalarRange(this->Data->GetScalarRange());
    this->ActorComposite->SetInput(geometry->GetOutput());
    geometry->Delete();
    }
  
  this->Actor->SetMapper(this->Mapper);
}

//----------------------------------------------------------------------------
vtkImageData* vtkPVImage::GetImageData()
{
  return (vtkImageData*)this->Data;
}

//----------------------------------------------------------------------------
void vtkPVImage::SetAssignment(vtkPVAssignment *a)
{
  if (this->Assignment == a)
    {
    return;
    }
  
  if (this->Assignment)
    {
    this->Assignment->UnRegister(NULL);
    this->Assignment = NULL;
    }

  if (a)
    {
    vtkImageData *image = this->GetImageData();
    
    if (image == NULL)
      {
      vtkErrorMacro("I do not have an image to make an assignment.");
      return;
      }
    this->Assignment = a;
    a->Register(this);
  
    image->SetUpdateExtent(a->GetExtent());
    }
}









