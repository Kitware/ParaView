/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageMandelbrotSource.h
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

#ifndef __vtkPVImageMandelbrotSource_h
#define __vtkPVImageMandelbrotSource_h

#include "vtkKWLabel.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWEntry.h"
#include "vtkKWPushButton.h"
#include "vtkImageMandelbrotSource.h"
#include "vtkPVImageSource.h"

class vtkPVImage;


class VTK_EXPORT vtkPVImageMandelbrotSource : public vtkPVImageSource
{
public:
  static vtkPVImageMandelbrotSource* New();
  vtkTypeMacro(vtkPVImageMandelbrotSource, vtkPVImageSource);

  // Description:
  // You will need to clone this object before you create it.
  void CreateProperties();
  
  // Description:
  // This method is used internally to cast the source to a vtkImageMandelbrotSource.
  vtkImageMandelbrotSource *GetImageMandelbrotSource();
  
  void AcceptParameters();

  // Description:
  // Parallel methods to set the parameters of the source.
  // All clones will get the same parameters.
  void SetDimensions(int dx, int dy, int dz);
  void SetSpacing(float sc, float sx);
  void SetCenter(float cReal, float cImage, float xReal, float xImag);
  
protected:
  vtkPVImageMandelbrotSource();
  ~vtkPVImageMandelbrotSource();
  vtkPVImageMandelbrotSource(const vtkPVImageMandelbrotSource&) {};
  void operator=(const vtkPVImageMandelbrotSource&) {};
  
  vtkKWPushButton *Accept;

  vtkKWLabeledFrame *DimensionsFrame;
  vtkKWLabel *XDimLabel;
  vtkKWEntry *XDimension;
  vtkKWLabel *YDimLabel;
  vtkKWEntry *YDimension;
  vtkKWLabel *ZDimLabel;
  vtkKWEntry *ZDimension;

  vtkKWLabeledFrame *CenterFrame;
  vtkKWLabel *CRealLabel;
  vtkKWEntry *CRealEntry;
  vtkKWLabel *CImaginaryLabel;
  vtkKWEntry *CImaginaryEntry;
  vtkKWLabel *XRealLabel;
  vtkKWEntry *XRealEntry;
  vtkKWLabel *XImaginaryLabel;
  vtkKWEntry *XImaginaryEntry;
  
  vtkKWLabel *CSpacingLabel;
  vtkKWEntry *CSpacingEntry;
  
  vtkKWLabel *XSpacingLabel;
  vtkKWEntry *XSpacingEntry;
  
  
};

#endif





