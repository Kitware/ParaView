/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageData.cxx
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
#include "vtkPVImageData.h"
#include "vtkPVImageClip.h"
#include "vtkPVImageSlice.h"
#include "vtkPVWindow.h"
#include "vtkImageOutlineFilter.h"
#include "vtkPVAssignment.h"
#include "vtkGeometryFilter.h"
#include "vtkKWMenuButton.h"
#include "vtkPVActorComposite.h"

int vtkPVImageDataCommand(ClientData cd, Tcl_Interp *interp,
		      int argc, char *argv[]);

//----------------------------------------------------------------------------
vtkPVImageData::vtkPVImageData()
{
  this->CommandFunction = vtkPVImageDataCommand;
}

//----------------------------------------------------------------------------
vtkPVImageData::~vtkPVImageData()
{
}

//----------------------------------------------------------------------------
vtkPVImageData* vtkPVImageData::New()
{
  return new vtkPVImageData();
}

//----------------------------------------------------------------------------
void vtkPVImageData::Clip()
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
  clip->AddInputList();
  
  clip->Delete();
}

//----------------------------------------------------------------------------
void vtkPVImageData::Slice()
{
  static int instanceCount = 0;
  vtkPVImageSlice *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();
  int *ext, d0, d1, d2;

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVImageSlice::SafeDownCast(
          pvApp->MakePVSource("vtkPVImageSlice",
                              "vtkImageClip", "Slice", ++instanceCount));
  if (f == NULL) {return;}
  f->SetInput(this);

  this->Script("%s ClipDataOn", f->GetVTKSourceTclName());

  // Lets try to setup a good default parameters.
  this->GetImageData()->UpdateInformation();
  ext = this->GetImageData()->GetWholeExtent();
  d0 = ext[1] - ext[0] + 1;
  d1 = ext[3] - ext[2] + 1;
  d2 = ext[5] - ext[4] + 1;
  // This seems kind of stupid.
  if (d0 < d1 && d0 < d2)
    {
    f->SetSliceNumber(ext[0]);
    f->SetSliceAxis(0);
    }
  else if (d1 < d2)
    {
    f->SetSliceNumber(ext[1]);
    f->SetSliceAxis(1);
    }
  else
    {
    f->SetSliceNumber((int)(((float)(ext[4]) + (float)(ext[5])) * 0.5));
    f->SetSliceAxis(2);
    }
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);
  f->AddInputList();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
void vtkPVImageData::ShiftScale()
{
  static int instanceCount = 0;
  vtkPVImageToImageFilter *f;
  vtkPVApplication *pvApp = this->GetPVApplication();
  vtkPVWindow *window = this->GetPVSource()->GetWindow();

  // Create the pvSource. Clone the PVSource and the vtkSource,
  // Link the PVSource to the vtkSource.
  f = vtkPVImageToImageFilter::SafeDownCast(
          pvApp->MakePVSource("vtkPVImageToImageFilter",
                              "vtkImageShiftScale", "ShiftScale", ++instanceCount));
  if (f == NULL) {return;}
  f->SetInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddInputList();
  f->AddLabeledEntry("Shift:", "SetShift", "GetShift");
  f->AddLabeledEntry("Scale:", "SetScale", "GetScale");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
}

//----------------------------------------------------------------------------
int vtkPVImageData::Create(char *args)
{
  if (this->vtkPVData::Create(args) == 0)
    {
    return 0;
    }
  
  this->FiltersMenuButton->AddCommand("vtkImageClip", this, "Clip");
  this->FiltersMenuButton->AddCommand("ImageSlice", this, "Slice");
  this->FiltersMenuButton->AddCommand("ShiftScale", this, "ShiftScale");
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImageData::SetData(vtkDataSet *data)
{
  vtkImageData *image = vtkImageData::SafeDownCast(data);
  int *ext;
  
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
  
  // Mode will tell the actor composite how to create the mapper.
  this->ActorComposite->SetInput(data);
  
  // Determine which mode to use.
  image->UpdateInformation();
  ext = image->GetWholeExtent();
  if (ext[1]==ext[0] || ext[3]==ext[2] || ext[5]==ext[4])
    {
    //this->ActorComposite->SetModeToDataSet();
    this->ActorComposite->SetModeToImageTexture();
    // Sets the scalar range...
    this->ActorComposite->Initialize();
    }
  else
    {
    this->ActorComposite->SetModeToImageOutline();
    // Sets the scalar range...
    this->ActorComposite->Initialize();
    }
}


//----------------------------------------------------------------------------
vtkImageData* vtkPVImageData::GetImageData()
{
  return (vtkImageData*)this->Data;
}

//----------------------------------------------------------------------------
void vtkPVImageData::SetAssignment(vtkPVAssignment *a)
{
  // This will take care of broadcasting the method
  this->vtkPVData::SetAssignment(a);
  
  if (a && this->GetImageData())
    {
    this->GetImageData()->SetExtentTranslator(a->GetTranslator());
    }
}


//----------------------------------------------------------------------------
void vtkPVImageData::Update()
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




