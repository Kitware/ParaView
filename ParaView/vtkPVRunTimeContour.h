/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVRunTimeContour.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
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

#ifndef __vtkPVRunTimeContour_h
#define __vtkPVRunTimeContour_h

#include "vtkPVPolyDataSource.h"
#include "vtkPVImageSource.h"
#include "vtkRunTimeContour.h"
#include "vtkPVImageData.h"

class VTK_EXPORT vtkPVRunTimeContour : public vtkPVPolyDataSource
{
public:
  static vtkPVRunTimeContour* New();
  vtkTypeMacro(vtkPVRunTimeContour, vtkPVSource);

  // Description:
  // Overriding the accept callback in PVSource
  void AcceptCallback();

  // Description:
  // These were copied from vtkPVImageSource, but I needed a copy of them
  // here since this class doesn't inherit from vtkPVImageSource.
  void InitializePVImageOutput(int idx);
  void SetNthPVImageOutput(int idx, vtkPVImageData *pvi);
  
protected:
  vtkPVRunTimeContour();
  ~vtkPVRunTimeContour() {};
  vtkPVRunTimeContour(const vtkPVRunTimeContour&) {};
  void operator=(const vtkPVRunTimeContour&) {};
};

#endif
