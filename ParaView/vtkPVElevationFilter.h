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

#include "vtkKWLabel.h"
#include "vtkElevationFilter.h"
#include "vtkKWEntry.h"
#include "vtkKWScale.h"
#include "vtkPVSource.h"

class vtkPVPolyData;
class vtkPVImage;


class VTK_EXPORT vtkPVElevationFilter : public vtkPVSource
{
public:
  static vtkPVElevationFilter* New();
  vtkTypeMacro(vtkPVElevationFilter, vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  int Create(char *args);

  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.  Do not try to
  // set the output before the input has been set.
  // This methods gets called in all processes.
  void SetOutput(vtkPVPolyData *pvd);
  void SetOutput(vtkPVImage *pvd);
  vtkPVData *GetOutput();
  vtkPVPolyData *GetPVPolyDataOutput();
  vtkPVImage *GetPVImageOutput();
  
  vtkGetObjectMacro(Elevation, vtkElevationFilter);
  
  void ElevationParameterChanged();

  // Description:
  // All pipeline calls have to use vtkKWObjects so GetTclName will work.
  // The methods executes on all processes.
  void SetInput(vtkPVData *pvData);

  // Description:
  // Filter parameters that get broadcast to all processes.
  void SetLowPoint(float x, float y, float z);
  void SetHighPoint(float x, float y, float z);
  void SetScalarRange(float min, float max);

  
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
  
  vtkKWEntry *LowPointXEntry;
  vtkKWEntry *LowPointYEntry;
  vtkKWEntry *LowPointZEntry;
  vtkKWEntry *HighPointXEntry;
  vtkKWEntry *HighPointYEntry;
  vtkKWEntry *HighPointZEntry;
  vtkKWEntry *RangeMinEntry;
  vtkKWEntry *RangeMaxEntry;
  vtkKWLabel *LowPointXLabel;
  vtkKWLabel *LowPointYLabel;
  vtkKWLabel *LowPointZLabel;
  vtkKWLabel *HighPointXLabel;
  vtkKWLabel *HighPointYLabel;
  vtkKWLabel *HighPointZLabel;
  vtkKWLabel *RangeMinLabel;
  vtkKWLabel *RangeMaxLabel;
  
  vtkKWWidget *Accept;

  vtkElevationFilter *Elevation;
};

#endif
