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
class vtkPVImageData;


class VTK_EXPORT vtkPVDataSetToDataSetFilter : public vtkPVSource
{
public:
  static vtkPVDataSetToDataSetFilter* New();
  vtkTypeMacro(vtkPVDataSetToDataSetFilter, vtkPVSource);

  // Description:
  // This method gets called when the accept button is pressed
  // for the first time.  It creates a pvData and its assignement.
  void InitializeOutput();  
  
  // Description:
  // Although the data is created in the initialize method,
  // this method is needed in the satellite processes to set the data.
  void SetPVOutput(vtkPVData *pvd);
  vtkPVData *GetPVOutput();
  vtkPVPolyData *GetPVPolyDataOutput();
  vtkPVImageData *GetPVImageDataOutput();
  
  // Description:
  // All pipeline calls have to use vtkKWObjects so GetTclName will work.
  // The methods executes on all processes.
  void SetInput(vtkPVData *pvData);
  
  vtkPVData *GetInput();
  
protected:
  vtkPVDataSetToDataSetFilter();
  ~vtkPVDataSetToDataSetFilter() {};
  vtkPVDataSetToDataSetFilter(const vtkPVDataSetToDataSetFilter&) {};
  void operator=(const vtkPVDataSetToDataSetFilter&) {};

  // Description:
  // Cast to the correct type.
  vtkDataSetToDataSetFilter *GetVTKDataSetToDataSetFilter();
};

#endif
