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
#include "vtkKWEntry.h"
#include "vtkKWLabel.h"
#include "vtkKWRadioButton.h"
#include "vtkPVSource.h"

class vtkPVImage;



class VTK_EXPORT vtkPVImageSlice : public vtkPVSource
{
public:
  static vtkPVImageSlice* New();
  vtkTypeMacro(vtkPVImageSlice, vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  int Create(char *args);
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetOutput(vtkPVImage *pvd);
  vtkPVImage *GetOutput();
  
  void SliceChanged();
  void SelectX();
  void SelectY();
  void SelectZ();

  vtkGetObjectMacro(Slice, vtkImageClip);
  
  void SetDimensions(int dim[6]);
  int *GetDimensions();
  
protected:
  vtkPVImageSlice();
  ~vtkPVImageSlice();
  vtkPVImageSlice(const vtkPVImageSlice&) {};
  void operator=(const vtkPVImageSlice&) {};
  
  vtkKWWidget *Accept;
  vtkKWEntry *SliceEntry;
  vtkKWLabel *SliceLabel;
  vtkKWRadioButton *XDimension;
  vtkKWRadioButton *YDimension;
  vtkKWRadioButton *ZDimension;
  
  vtkImageClip  *Slice;
  int Dimensions[6];
};

#endif
