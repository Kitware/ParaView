/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataSource.h
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
// .NAME vtkPVPolyDataSource - PV equivavlent to vtkPolyDataSource.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.

#ifndef __vtkPVPolyDataSource_h
#define __vtkPVPolyDataSource_h

#include "vtkPolyDataSource.h"
#include "vtkPVSource.h"

class vtkPVPolyData;


class VTK_EXPORT vtkPVPolyDataSource : public vtkPVSource
{
public:
  static vtkPVPolyDataSource* New();
  vtkTypeMacro(vtkPVPolyDataSource, vtkPVSource);

  // Description:
  // Access to the underlying vtk poly data source.
  // This access should probably not exist.  Every thing should
  // be done through the PV interface so that all
  // clones can be syncronized.
  vtkGetObjectMacro(PolyDataSource, vtkPolyDataSource);

  // Description:
  // This method gets called when the accept button is pressed
  // for the first time.  It creates a pvData and an actor composite
  // to display the data.
  void InitializeData();

  // Description:
  // This method is called when the GetSource button is pressed.
  // It makes the input source the current source.
  void SelectInputSource();
  
  // Description:
  // The user has to set the output explicitly.
  // This is executed in all processes.
  void SetOutput(vtkPVPolyData *pvd);
  vtkPVPolyData *GetOutput();
  
protected:
  vtkPVPolyDataSource();
  ~vtkPVPolyDataSource();
  vtkPVPolyDataSource(const vtkPVPolyDataSource&) {};
  void operator=(const vtkPVPolyDataSource&) {};

  // Description:
  // Convenience method for reference counting.
  vtkSetObjectMacro(PolyDataSource, vtkPolyDataSource);
  
  vtkPolyDataSource *PolyDataSource;
};

#endif
