/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageReader.h
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

#ifndef __vtkPVImageReader_h
#define __vtkPVImageReader_h

#include "vtkKWLabeledEntry.h"
#include "vtkKWPushButton.h"
#include "vtkImageReader.h"
#include "vtkPVImageSource.h"

class vtkPVImageData;


class VTK_EXPORT vtkPVImageReader : public vtkPVImageSource
{
public:
  static vtkPVImageReader* New();
  vtkTypeMacro(vtkPVImageReader, vtkPVImageSource);

  // Description:
  // You will need to clone this object before you create it.
  void CreateProperties();
  
  // Description:
  // This method is used internally to cast the source to a vtkImageReader.
  vtkImageReader *GetImageReader();
  
  void ImageAccepted();
  void OpenFile();

  // Description:
  // Parallel methods to set the parameters of the reader.
  // All clones should have the same parameters.
  void SetDataByteOrder(int o);
  void SetDataExtent(int xmin,int xmax, int ymin,int ymax, int zmin,int zmax);
  void SetDataSpacing(float sx, float sy, float sz);
  void SetFilePrefix(char *prefix);
  
protected:
  vtkPVImageReader();
  ~vtkPVImageReader();
  vtkPVImageReader(const vtkPVImageReader&) {};
  void operator=(const vtkPVImageReader&) {};
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *Open;
  vtkKWLabeledEntry *XDimension;
  vtkKWLabeledEntry *YDimension;
  vtkKWLabeledEntry *ZDimension;
};

#endif
