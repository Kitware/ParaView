/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImage.h
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
// .NAME vtkPVImage - ImageData interface.
// .SECTION Description
// This is a parallel object.  It needs to be cloned to work correctly.  
// After cloning, the parallel nature of the object is transparent.


#ifndef __vtkPVImage_h
#define __vtkPVImage_h

#include "vtkKWWidget.h"
#include "vtkProp.h"
#include "vtkImageData.h"
#include "vtkOutlineSource.h"
#include "vtkPVData.h"

class vtkTexture;
class vtkPVComposite;
class vtkPVImageTextureFilter;

class VTK_EXPORT vtkPVImage : public vtkPVData
{
public:
  static vtkPVImage* New();
  vtkTypeMacro(vtkPVImage,vtkKWWidget);
  
  // Description:
  // You have to clone this object before you create it.
  int Create(char *args);
  
  // Description:
  // When this ivar is tru, then an outline is used to represent the image.
  vtkSetMacro(OutlineFlag,int);
  vtkGetMacro(OutlineFlag,int);
  vtkBooleanMacro(OutlineFlag,int);

  void SetImageData(vtkImageData *image);
  vtkImageData *GetImageData();
  
  void Clip();
  void Slice();
  
  // Description:
  // Like update extent, but an object tells which piece to assign this process.
  void SetAssignment(vtkPVAssignment *a);

protected:
  vtkPVImage();
  ~vtkPVImage();
  vtkPVImage(const vtkPVImage&) {};
  void operator=(const vtkPVImage&) {};
  
  int OutlineFlag;
  
  vtkPVImageTextureFilter *TextureFilter;
  vtkTexture *Texture;
};

#endif
