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
#include "vtkPVMenuButton.h"
#include "vtkPVActorComposite.h"

int vtkPVImageCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImage::vtkPVImage()
{
  this->OutlineFlag = 1;
  this->CommandFunction = vtkPVImageCommand;
  this->GeometryFilter = NULL;
}

//----------------------------------------------------------------------------
vtkPVImage::~vtkPVImage()
{
  if (this->GeometryFilter)
    {
    this->GeometryFilter->Delete();
    this->GeometryFilter = NULL;
    }
}

//----------------------------------------------------------------------------
vtkPVImage* vtkPVImage::New()
{
  return new vtkPVImage();
}



//----------------------------------------------------------------------------
void vtkPVImage::SetOutlineFlag(int f)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  if (this->OutlineFlag == f)
    {
    return;
    }
  this->Modified();
  this->OutlineFlag = f;
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutlineFlag %d", this->GetTclName(), f);
    }  
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
  
  slice = vtkPVImageSlice::New();
  slice->Clone(pvApp);

  slice->SetInput(this);  
  slice->SetName("slice");
  
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  this->GetPVSource()->GetView()->AddComposite(slice);
  
  window->SetCurrentSource(slice);
  window->GetSourceList()->Update();
  
  // Lets try to setup a good default parameters.
  this->GetImageData()->UpdateInformation();
  int *ext = this->GetImageData()->GetWholeExtent();
  if (ext[0] == ext[1])
    {
    slice->SetSliceNumber(ext[0]);
    slice->SetSliceAxis(0);
    }
  else if (ext[2] == ext[3])
    {
    slice->SetSliceNumber(ext[1]);
    slice->SetSliceAxis(1);
    }
  else
    {
    slice->SetSliceNumber((int)(((float)(ext[4]) + (float)(ext[5])) * 0.5));
    slice->SetSliceAxis(2);
    }
  
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
void vtkPVImage::SetData(vtkDataSet *data)
{
  vtkImageOutlineFilter *outline;
  vtkImageData *image = vtkImageData::SafeDownCast(data);
  
  if (data != NULL && image == NULL)
    {
    vtkErrorMacro("Expecting an image");
    return;
    }
  
  this->vtkPVData::SetData(data);
  
  if (this->Assignment)
    {
    image->SetExtentTranslator(this->Assignment->GetTranslator());
    }
  
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
    if (this->GeometryFilter == NULL)
      {
      this->GeometryFilter = vtkGeometryFilter::New();
      }
    this->GeometryFilter->SetInput(image);
    this->Mapper->SetInput(this->GeometryFilter->GetOutput());
    this->Mapper->SetScalarRange(this->Data->GetScalarRange());
    this->ActorComposite->SetInput(this->GeometryFilter->GetOutput());
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
  // This will take care of broadcasting the method
  this->vtkPVData::SetAssignment(a);
  
  if (a && this->GetImageData())
    {
    this->GetImageData()->SetExtentTranslator(a->GetTranslator());
    }
  
}


//----------------------------------------------------------------------------
void vtkPVImage::Update()
{
  vtkImageData *image;

  if (this->Data == NULL)
    {
    vtkErrorMacro("No data object to update.");
    }

  image = this->GetImageData();
  image->UpdateInformation();
  if (this->Assignment)
    {
    this->Assignment->SetWholeExtent(image->GetWholeExtent());
    image->SetUpdateExtent(this->Assignment->GetExtent());
    }

  image->Update();
}






