/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVPolyDataNormals.h
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

#ifndef __vtkPVPolyDataNormals_h
#define __vtkPVPolyDataNormals_h

#include "vtkPolyDataNormals.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWLabel.h"
#include "vtkKWEntry.h"
#include "vtkPVSource.h"

class vtkPVPolyData;
class vtkPVImage;


class VTK_EXPORT vtkPVPolyDataNormals : public vtkPVSource
{
public:
  static vtkPVPolyDataNormals* New();
  vtkTypeMacro(vtkPVPolyDataNormals, vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();

  // Description:
  // This method executes in every process.
  void SetInput(vtkPVData *input);
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetOutput(vtkPVPolyData *pvd);
  vtkPVPolyData *GetOutput();
  
  // Description:
  // Get the underlying vtk filter
  vtkGetObjectMacro(PolyDataNormals, vtkPolyDataNormals);
  
  void NormalsParameterChanged();
  
  // Description:
  // These methods execute in every process.
  void SetSplitting(int split);
  void SetFeatureAngle(float angle);
  
protected:
  vtkPVPolyDataNormals();
  ~vtkPVPolyDataNormals();
  vtkPVPolyDataNormals(const vtkPVPolyDataNormals&) {};
  void operator=(const vtkPVPolyDataNormals&) {};
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *SourceButton;
  vtkKWCheckButton *Splitting;
  vtkKWLabel *FeatureAngleLabel;
  vtkKWEntry *FeatureAngleEntry;
  
  vtkPolyDataNormals *PolyDataNormals;
};

#endif
