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


class VTK_EXPORT vtkPVElevationFilter : public vtkPVSource
{
public:
  static vtkPVElevationFilter* New();
  vtkTypeMacro(vtkPVElevationFilter, vtkPVSource);

  void Create(vtkKWApplication *app, char *args);

  vtkGetObjectMacro(Elevation, vtkElevationFilter);
  
  void ElevationParameterChanged();
  
protected:
  vtkPVElevationFilter();
  ~vtkPVElevationFilter();
  vtkPVElevationFilter(const vtkPVElevationFilter&) {};
  void operator=(const vtkPVElevationFilter&) {};
    
  vtkKWLabel *Label;
  
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
