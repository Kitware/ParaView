/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVDataSetToDataSetFilter.h
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

#ifndef __vtkPVDataSetToDataSetFilter_h
#define __vtkPVDataSetToDataSetFilter_h

#include "vtkKWLabel.h"
#include "vtkDataSetToDataSetFilter.h"
#include "vtkKWPushButton.h"
#include "vtkPVSource.h"

class vtkPVPolyData;
class vtkPVImage;


class VTK_EXPORT vtkPVDataSetToDataSetFilter : public vtkPVSource
{
public:
  static vtkPVDataSetToDataSetFilter* New();
  vtkTypeMacro(vtkPVDataSetToDataSetFilter, vtkPVSource);

  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.  Do not try to
  // set the output before the input has been set.
  // This methods gets called in all processes.
  void SetOutput(vtkPVData *pvd);
  vtkPVData *GetOutput();
  vtkPVPolyData *GetPVPolyDataOutput();
  vtkPVImage *GetPVImageOutput();
  
  // Description:
  // Make the input source the current composite.
  void SelectInputSource();
  
  // Description:
  // All pipeline calls have to use vtkKWObjects so GetTclName will work.
  // The methods executes on all processes.
  void SetInput(vtkPVData *pvData);

protected:
  vtkPVDataSetToDataSetFilter();
  ~vtkPVDataSetToDataSetFilter();
  vtkPVDataSetToDataSetFilter(const vtkPVDataSetToDataSetFilter&) {};
  void operator=(const vtkPVDataSetToDataSetFilter&) {};

  // Description:
  // This method is called the first time the accept button is pressed.
  // It creates the pvData object, and the actor composite for display.
  void InitializeData();
  
  
  // Description:
  // A convenience method for setting the filter.
  vtkSetObjectMacro(Filter, vtkDataSetToDataSetFilter);
  
  vtkDataSetToDataSetFilter *Filter;
};

#endif
