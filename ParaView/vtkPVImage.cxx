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
#include "vtkPVComposite.h"
#include "vtkPVWindow.h"
#include "vtkOutlineFilter.h"
#include "vtkPVAssignment.h"

int vtkPVImageCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImage::vtkPVImage()
{
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
  vtkPVImageClip *clip;
  vtkPVComposite *newComp;
  int *extents;
  
  extents = this->GetImageData()->GetExtent();
  extents[5] = (extents[4] + extents[5])/2;
  extents[4] = extents[5];
  
  clip = vtkPVImageClip::New();
  clip->GetImageClip()->SetInput(this->GetImageData());
  
  clip->GetImageClip()->ClipDataOn();
  clip->GetImageClip()->SetOutputWholeExtent(extents);
  clip->GetImageClip()->Update();

  newComp = vtkPVComposite::New();
  newComp->SetSource(clip);
  newComp->SetCompositeName("clip");
  
  vtkPVWindow *window = this->GetComposite()->GetWindow();
  newComp->SetPropertiesParent(window->GetDataPropertiesParent());
  newComp->CreateProperties("");
  this->GetComposite()->GetView()->AddComposite(newComp);
  this->GetComposite()->VisibilityOff();
  
  newComp->SetWindow(window);
  
  window->SetCurrentDataComposite(newComp);
  window->GetDataList()->Update();

  window->GetMainView()->ResetCamera();
  window->GetMainView()->Render();
  
  this->GetComposite()->GetView()->Render();
  
  newComp->Delete();
  clip->Delete();
}

//----------------------------------------------------------------------------
void vtkPVImage::Slice()
{
  vtkPVImageSlice *slice;
  vtkPVComposite *newComp;
  int *extents;
  
  slice = vtkPVImageSlice::New();
  slice->GetSlice()->SetInput(this->GetImageData());
  
  extents = this->GetImageData()->GetExtent();
  slice->SetDimensions(extents);
  
  extents[5] = (extents[4] + extents[5])/2;
  extents[4] = extents[5];
  
  slice->GetSlice()->ClipDataOn();
  slice->GetSlice()->SetOutputWholeExtent(extents);
  slice->GetSlice()->Update();

  newComp = vtkPVComposite::New();
  newComp->SetSource(slice);
  newComp->SetCompositeName("slice");
  
  vtkPVWindow *window = this->GetComposite()->GetWindow();
  newComp->SetPropertiesParent(window->GetDataPropertiesParent());
  newComp->CreateProperties("");
  this->GetComposite()->GetView()->AddComposite(newComp);
  this->GetComposite()->VisibilityOff();
  
  newComp->SetWindow(window);
  
  window->SetCurrentDataComposite(newComp);
  window->GetDataList()->Update();
  
  this->GetComposite()->GetView()->Render();
  
  newComp->Delete();
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
void vtkPVImage::SetImageData(vtkImageData *data)
{
  vtkOutlineFilter *outline;
  int *dim;
  
  data->Update();
  dim = data->GetDimensions();
  outline = vtkOutlineFilter::New();
  outline->SetInput(data);
  
  this->SetData(data);
  if ((dim[0] > 1) && (dim[1] > 1) && (dim[2] > 1))
    {
    this->Mapper->SetInput(outline->GetOutput());
    }
  else
    {
    this->Mapper->SetInput(data);
    }
  
  this->Data->Update();
  this->Mapper->SetScalarRange(this->Data->GetScalarRange());
  this->Actor->SetMapper(this->Mapper);
  
  outline->Delete();
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









