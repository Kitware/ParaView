/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVShrinkPolyData.h
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

#ifndef __vtkPVShrinkPolyData_h
#define __vtkPVShrinkPolyData_h

#include "vtkKWLabel.h"
#include "vtkShrinkPolyData.h"
#include "vtkKWScale.h"
#include "vtkPVSource.h"

class vtkPVPolyData;


class VTK_EXPORT vtkPVShrinkPolyData : public vtkPVSource
{
public:
  static vtkPVShrinkPolyData* New();
  vtkTypeMacro(vtkPVShrinkPolyData,vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetOutput(vtkPVPolyData *pvd);
  vtkPVPolyData *GetOutput();
  
  void ShrinkFactorChanged();

  vtkGetObjectMacro(Shrink, vtkShrinkPolyData);

  // Description:
  // The methods execute on all processes.
  void SetInput(vtkPVPolyData *pvData);
  
  // Description:
  // The methods execute on all processes.
  void SetShrinkFactor(float factor);

protected:
  vtkPVShrinkPolyData();
  ~vtkPVShrinkPolyData();
  vtkPVShrinkPolyData(const vtkPVShrinkPolyData&) {};
  void operator=(const vtkPVShrinkPolyData&) {};
  
  vtkKWWidget *Accept;
  vtkKWScale *ShrinkFactorScale;

  vtkShrinkPolyData *Shrink;
};

#endif
