/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTubeFilter.h
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

#ifndef __vtkPVTubeFilter_h
#define __vtkPVTubeFilter_h

#include "vtkKWLabel.h"
#include "vtkTubeFilter.h"
#include "vtkKWEntry.h"
#include "vtkKWPushButton.h"
#include "vtkPVPolyDataToPolyDataFilter.h"

class vtkPVPolyData;


class VTK_EXPORT vtkPVTubeFilter : public vtkPVPolyDataToPolyDataFilter
{
public:
  static vtkPVTubeFilter* New();
  vtkTypeMacro(vtkPVTubeFilter, vtkPVPolyDataToPolyDataFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();
  
  // Description:
  // This method is called when the accpt button is pressed.
  void TubeFilterChanged();

  // Description:
  // This is used internally to cast the source to a vtkTubeFilter
  vtkTubeFilter *GetTubeFilter();

  // Description:
  // The methods execute on all processes.
  void SetRadius(float radius);
  void SetNumberOfSides(int sides);

protected:
  vtkPVTubeFilter();
  ~vtkPVTubeFilter();
  vtkPVTubeFilter(const vtkPVTubeFilter&) {};
  void operator=(const vtkPVTubeFilter&) {};
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *SourceButton;
  vtkKWLabel *RadiusLabel;
  vtkKWEntry *RadiusEntry;
  vtkKWLabel *SidesLabel;
  vtkKWEntry *SidesEntry;
};

#endif
