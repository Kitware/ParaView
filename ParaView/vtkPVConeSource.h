/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVConeSource.h
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
// .NAME vtkPVConeSource - Parallel cone source with interface.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.

#ifndef __vtkPVConeSource_h
#define __vtkPVConeSource_h

#include "vtkPVPolyDataSource.h"

class vtkPVPolyData;


class VTK_EXPORT vtkPVConeSource : public vtkPVPolyDataSource
{
public:
  static vtkPVConeSource* New();
  vtkTypeMacro(vtkPVConeSource,vtkPVPolyDataSource);

  // Description:
  // You have to clone this object before you can create it.
  void CreateProperties();

protected:
  vtkPVConeSource();
  ~vtkPVConeSource() {};
  vtkPVConeSource(const vtkPVConeSource&) {};
  void operator=(const vtkPVConeSource&) {};
};

#endif
