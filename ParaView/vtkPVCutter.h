/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVCutter.h
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

#ifndef __vtkPVCutter_h
#define __vtkPVCutter_h

#include "vtkPVDataSetToPolyDataFilter.h"
#include "vtkCutter.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkInteractorStylePlane.h"

class vtkPVPolyData;
class vtkPVImageData;


class VTK_EXPORT vtkPVCutter : public vtkPVDataSetToPolyDataFilter
{
public:
  static vtkPVCutter* New();
  vtkTypeMacro(vtkPVCutter, vtkPVDataSetToPolyDataFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();
    
  // Description:
  // These set the parameters of the plane.
  void SetOrigin(float originX, float originY, float originZ);
  vtkGetVector3Macro(Origin, float);
  void SetNormal(float normalX, float normalY, float normalZ);
  vtkGetVector3Macro(Normal, float);
  
  vtkGetObjectMacro(PlaneStyle, vtkInteractorStylePlane);
  void UsePlaneStyle();
 
  // Description:
  // need to pack/unpack the plane interactor style button depending on
  // whether we are selecting or deselecting this source
  virtual void Select(vtkKWView *view);
  virtual void Deselect(vtkKWView *view);
  
  // Description:
  // Callback for checkbutton to determine whether to display the crosshair
  // that shows where the plane is
  void CrosshairDisplay();
  
protected:
  vtkPVCutter();
  ~vtkPVCutter();
  vtkPVCutter(const vtkPVCutter&) {};
  void operator=(const vtkPVCutter&) {};
  
  vtkKWCheckButton *ShowCrosshairButton;

  vtkKWPushButton *PlaneStyleButton;
  
  vtkInteractorStylePlane *PlaneStyle;

  int PlaneStyleCreated;
  int PlaneStyleUsed;

  // These duplicates are here because I did not want to modify the hints file.
  float Origin[3];
  float Normal[3];
};

#endif
