/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSlice.h
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

#ifndef __vtkPVImageSlice_h
#define __vtkPVImageSlice_h

#include "vtkImageClip.h"
#include "vtkPVImageToImageFilter.h"
#include "vtkInteractorStyleImageExtent.h"

class vtkPVImageData;
class vtkKWScale;
class vtkPVSelectionList;

class VTK_EXPORT vtkPVImageSlice : public vtkPVImageToImageFilter
{
public:
  static vtkPVImageSlice* New();
  vtkTypeMacro(vtkPVImageSlice, vtkPVImageToImageFilter);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();
    
  // Description:
  // Manual method for selecting which axis gets sliced.
  void SetSliceAxis(int axis);
  vtkGetMacro(SliceAxis, int);

  // Description:
  // Manual method for selecting which slice to display
  void SetSliceNumber(int slice);
  vtkGetMacro(SliceNumber, int);
  
  // Description:
  // Get the underlying vtkImageSlice
  vtkImageClip *GetImageClip();
  
  vtkGetObjectMacro(SliceStyle, vtkInteractorStyleImageExtent);
  
  // Description:
  // overriding the Select/Deselect methods in the superclass so that we can
  // add / remove the slice style button from the toolbar when this filter is
  // selected / deselected
  virtual void Select(vtkKWView *view);
  virtual void Deselect(vtkKWView *view);
  
  void UseSliceStyle();

  // Description:
  // A callback from the axis selection list that
  // changes the rang of the slice scale.
  void ComputeSliceRange();
  
protected:
  vtkPVImageSlice();
  ~vtkPVImageSlice();
  vtkPVImageSlice(const vtkPVImageSlice&) {};
  void operator=(const vtkPVImageSlice&) {};
  
  // This is called to update the widgets when the parameters are changed
  // programatically (not through UI).
  void UpdateVTKSource();
  
  // Keep the parameters here (in addition to the widgets)
  // so they can be set before the properties are created.
  int SliceNumber;
  int SliceAxis;
  
  // We need to keep a reference to these to change the extent of the scale
  // when the axis changes.

  vtkInteractorStyleImageExtent *SliceStyle;
  vtkKWPushButton *SliceStyleButton;
  int SliceStyleCreated;

  // Axis affects scale.
  vtkKWScale *SliceScale;
  vtkPVSelectionList *AxisWidget;
};

#endif
