/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageMandelbrotSource.cxx
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

#include "vtkPVImageMandelbrotSource.h"
#include "vtkImageMandelbrotSource.h"

//----------------------------------------------------------------------------
vtkPVImageMandelbrotSource::vtkPVImageMandelbrotSource()
{
  vtkImageMandelbrotSource *s = vtkImageMandelbrotSource::New();
  s->SetMaximumNumberOfIterations(200);
  this->SetVTKSource(s);

  // initialize parameters
  //s->SetCenter(-0.733569, 0.24405, 0.296116, 0.0253163);
  //s->SetSpacing(1.38125e-005, 1.0e-004);
  //s->SetDimensions(110, 110, 110);
  
  s->Delete();
}

//----------------------------------------------------------------------------
vtkPVImageMandelbrotSource* vtkPVImageMandelbrotSource::New()
{
  return new vtkPVImageMandelbrotSource();
}

//----------------------------------------------------------------------------
void vtkPVImageMandelbrotSource::CreateProperties()
{  
  this->vtkPVSource::CreateProperties();

  //void SetDimensions(int dx, int dy, int dz);
  //void SetSpacing(float sc, float sx);
  //void SetCenter(float cReal, float cImage, float xReal, float xImag);

}

