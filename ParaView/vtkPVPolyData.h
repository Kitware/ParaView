/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyData.h
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
// .NAME vtkPVPolyData - PolyData interface.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.

#ifndef __vtkPVPolyData_h
#define __vtkPVPolyData_h

#include "vtkPolyData.h"
#include "vtkDataSet.h"
#include "vtkPVData.h"

class VTK_EXPORT vtkPVPolyData : public vtkPVData
{
public:
  static vtkPVPolyData* New();
  vtkTypeMacro(vtkPVPolyData, vtkKWWidget);
  
  // Description:
  // Just like in vtk data objects, this method makes a data object
  // that is of the same type as the original.  It is used for creating
  // the output pvData in pvDataSetToDataSetFilters.
  vtkPVData *MakeObject() {return vtkPVPolyData::New();}

  // Description:
  // You have to clone this object before you create it.
  int Create(char *args);

  // Description:
  // Source uses this method to set the VTK data object.
  void SetData(vtkDataSet *data);
  vtkPolyData *GetPolyData();
  
  void Shrink();
  void Glyph();
  void GetGhostCells();
  void PolyDataNormals();

protected:
  vtkPVPolyData();
  ~vtkPVPolyData();
  vtkPVPolyData(const vtkPVPolyData&) {};
  void operator=(const vtkPVPolyData&) {};
};

#endif

