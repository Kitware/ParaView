/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataToPolyDataFilter.h
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

#ifndef __vtkPVPolyDataToPolyDataFilter_h
#define __vtkPVPolyDataToPolyDataFilter_h

#include "vtkKWLabel.h"
#include "vtkShrinkPolyData.h"
#include "vtkKWScale.h"
#include "vtkKWPushButton.h"
#include "vtkPVPolyDataSource.h"

class vtkPVPolyData;


class VTK_EXPORT vtkPVPolyDataToPolyDataFilter : public vtkPVPolyDataSource
{
public:
  static vtkPVPolyDataToPolyDataFilter* New();
  vtkTypeMacro(vtkPVPolyDataToPolyDataFilter,vtkPVPolyDataSource);

  // Description:
  // The methods execute on all processes.
  void SetInput(vtkPVPolyData *pvData);
  
protected:
  vtkPVPolyDataToPolyDataFilter();
  ~vtkPVPolyDataToPolyDataFilter() {};
  vtkPVPolyDataToPolyDataFilter(const vtkPVPolyDataToPolyDataFilter&) {};
  void operator=(const vtkPVPolyDataToPolyDataFilter&) {};
};

#endif
