/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVElevationFilter.h
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

#ifndef __vtkPVElevationFilter_h
#define __vtkPVElevationFilter_h

#include "vtkPVDataSetToDataSetFilter.h"
#include "vtkElevationFilter.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkPVSource.h"

class vtkPVPolyData;
class vtkPVImage;


class VTK_EXPORT vtkPVElevationFilter : public vtkPVDataSetToDataSetFilter
{
public:
  static vtkPVElevationFilter* New();
  vtkTypeMacro(vtkPVElevationFilter, vtkPVDataSetToDataSetFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();

  // Description:
  // this method casts the fitler to an elevation filter.
  vtkElevationFilter *GetElevation();
  
  // Description:
  // A callback that gets called when the Accept button is pressed.
  void ElevationParameterChanged();
  
  // Description:
  // Filter parameters that get broadcast to all processes.
  void SetLowPoint(float x, float y, float z);
  void SetHighPoint(float x, float y, float z);
  void SetScalarRange(float min, float max);

  void GetSource();
  
protected:
  vtkPVElevationFilter();
  ~vtkPVElevationFilter();
  vtkPVElevationFilter(const vtkPVElevationFilter&) {};
  void operator=(const vtkPVElevationFilter&) {};
  
  vtkKWLabel *LowPointLabel;
  vtkKWLabel *HighPointLabel;
  vtkKWLabel *RangeLabel;
  vtkKWWidget *LowPointFrame;
  vtkKWWidget *HighPointFrame;
  vtkKWWidget *RangeFrame;
  
  vtkKWLabeledEntry *LowPointXEntry;
  vtkKWLabeledEntry *LowPointYEntry;
  vtkKWLabeledEntry *LowPointZEntry;
  vtkKWLabeledEntry *HighPointXEntry;
  vtkKWLabeledEntry *HighPointYEntry;
  vtkKWLabeledEntry *HighPointZEntry;
  vtkKWLabeledEntry *RangeMinEntry;
  vtkKWLabeledEntry *RangeMaxEntry;
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *SourceButton;
};

#endif
