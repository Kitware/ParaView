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
#include "vtkKWApplication.h"

vtkPVImageReader::vtkPVImageReader()
{
  this->Label = vtkKWLabel::New();
  this->ImageReader = vtkImageReader::New();
}

vtkPVImageReader::~vtkPVImageReader()
{
  this->Label->Delete();
  this->Label = NULL;
  
  this->ImageReader->Delete();
  this->ImageReader = NULL;
}

vtkPVImageReader* vtkPVImageReader::New()
{
  return new vtkPVImageReader();
}

void vtkPVImageReader::Create(vtkKWApplication *app, char *args)
{  
  // must set the application
  if (this->Application)
    {
    vtkErrorMacro("vtkPVImageReader already created");
    return;
    }
  this->SetApplication(app);
  
  // create the top level
  this->Script("frame %s %s", this->GetWidgetName(), args);
  
  this->Label->SetParent(this);
  this->Label->Create(this->Application, "");
  this->Label->SetLabel("vtkPVImageReader label");
  this->Script("pack %s", this->Label->GetWidgetName());
}

void vtkPVImageReader::ReadImage()
{
  this->ImageReader->SetDataByteOrderToLittleEndian();
  this->ImageReader->SetDataExtent(0, 63, 0, 63, 1, 93);
  this->ImageReader->SetFilePrefix("../../vtkdata/headsq/quarter");
  this->ImageReader->SetDataSpacing(4, 4, 1.8);
}
