/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSource.cxx
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

#include "vtkPVApplication.h"
#include "vtkPVImage.h"
#include "vtkPVActorComposite.h"
#include "vtkPVWindow.h"
#include "vtkPVAssignment.h"

//----------------------------------------------------------------------------
vtkPVImageSource::vtkPVImageSource()
{
  this->ImageSource = NULL;  
}

//----------------------------------------------------------------------------
vtkPVImageSource::~vtkPVImageSource()
{
  this->SetImageSource(NULL);
}

//----------------------------------------------------------------------------
vtkPVImageSource* vtkPVImageSource::New()
{
  return new vtkPVImageSource();
}



//----------------------------------------------------------------------------
void vtkPVImageSource::SetOutput(vtkPVImage *pvi)
{
  vtkPVApplication *pvApp = this->GetPVApplication();

  this->SetPVData(pvi);
  pvi->SetData(this->ImageSource->GetOutput());
  
  if (pvApp && pvApp->GetController()->GetLocalProcessId() == 0)
    {
    pvApp->BroadcastScript("%s SetOutput %s", this->GetTclName(), 
			   pvi->GetTclName());
    }
}

//----------------------------------------------------------------------------
vtkPVImage *vtkPVImageSource::GetOutput()
{
  return vtkPVImage::SafeDownCast(this->Output);
}

//----------------------------------------------------------------------------
void vtkPVImageSource::InitializeData()
{

    pvImage = vtkPVImage::New();
    pvImage->Clone(pvApp);
    a = vtkPVAssignment::New();
    a->Clone(pvApp);
    
    this->SetOutput(pvImage);
    // It is important that the pvImage have its image data befor this call.
    // The assignments vtk object is vtkPVExtentTranslator which needs the image.
    // This may get resolved as more functionality of Assignement gets into ExtentTranslator.
    // Maybe the pvImage should create the vtkIamgeData, and the source will set it as its output.
    a->SetOriginalImage(pvImage);
    
    pvImage->SetAssignment(a);
    
    this->CreateDataPage();
  
    ac = this->GetPVData()->GetActorComposite();
    window->GetMainView()->AddComposite(ac);
}

