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
#include "vtkPVWindow.h"
#include "vtkImageOutlineFilter.h"
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
}

//----------------------------------------------------------------------------
void vtkPVImageData::Slice()
{
}

//----------------------------------------------------------------------------
void vtkPVImageData::ShiftScale()
{
  /*
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
  f->SetPVInput(this);
  
  // Add the new Source to the View, and make it current.
  this->GetPVSource()->GetView()->AddComposite(f);
  window->SetCurrentSource(f);

  // Add some source specific widgets.
  // Normally these would be added in the CreateProperties method.
  f->AddPVInputList();
  f->AddLabeledEntry("Shift:", "SetShift", "GetShift");
  f->AddLabeledEntry("Scale:", "SetScale", "GetScale");
  f->UpdateParameterWidgets();

  // Clean up. (How about on the other processes?)
  // We cannot create an object in tcl and delete it in C++.
  //f->Delete();
  */
}

//----------------------------------------------------------------------------
int vtkPVImageData::Create(char *args)
{
  if (this->vtkPVData::Create(args) == 0)
    {
    return 0;
    }
  
  return 1;
}

//----------------------------------------------------------------------------
void vtkPVImageData::SetVTKData(vtkDataSet *data)
{
  vtkImageData *image = vtkImageData::SafeDownCast(data);
  int *ext;
  int id = this->GetPVApplication()->GetController()->GetLocalProcessId();

  if (data != NULL && image == NULL)
    {
    vtkErrorMacro("Expecting an image");
    return;
    }
  
  this->vtkPVData::SetVTKData(data);
  
  // Mode will tell the actor composite how to create the mapper.
  //this->ActorComposite->SetInput(data);
  
  // Determine which mode to use.
  image->UpdateInformation();
  ext = image->GetWholeExtent();
  if (ext[1]==ext[0] || ext[3]==ext[2] || ext[5]==ext[4])
    {
    //this->ActorComposite->SetModeToDataSet();
    this->ActorComposite->SetModeToImageTexture();
    }
  else
    {
    this->ActorComposite->SetModeToImageOutline();
    }
}


//----------------------------------------------------------------------------
vtkImageData* vtkPVImageData::GetVTKImageData()
{
  return (vtkImageData*)this->VTKData;
}

//----------------------------------------------------------------------------
void vtkPVImageData::Update()
{
  vtkImageData *image;

  if (this->VTKData == NULL)
    {
    vtkErrorMacro("No data object to update.");
    }

  image = this->GetVTKImageData();
  image->UpdateInformation();
  image->Update();
}




