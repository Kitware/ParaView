/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSingleContourFilter.h
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
// .NAME vtkSingleContourFilter - One contour for easy UI.
// .SECTION Description
// We may have a way to add contours in the future, 
// but now you can have only one.

#ifndef __vtkSingleContourFilter_h
#define __vtkSingleContourFilter_h

#include "vtkKitwareContourFilter.h"


class VTK_EXPORT vtkSingleContourFilter : public vtkKitwareContourFilter
{
public:
  static vtkSingleContourFilter* New();
  vtkTypeMacro(vtkSingleContourFilter, vtkKitwareContourFilter);

  // Description:
  // Add value would not work.  We need a simple interface.
  void SetFirstValue(float val);
  float GetFirstValue();

protected:
  vtkSingleContourFilter() {};
  ~vtkSingleContourFilter() {};
  vtkSingleContourFilter(const vtkSingleContourFilter&) {};
  void operator=(const vtkSingleContourFilter&) {};
};

#endif
