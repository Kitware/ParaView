/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageSource.h
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

#ifndef __vtkPVImageSource_h
#define __vtkPVImageSource_h

#include "vtkImageReader.h"
#include "vtkPVSource.h"

class vtkPVImageData;


class VTK_EXPORT vtkPVImageSource : public vtkPVSource
{
public:
  static vtkPVImageSource* New();
  vtkTypeMacro(vtkPVImageSource, vtkPVSource);

  // Description:
  // This method gets called when the accept button is pressed
  // for the first time.  It creates a pvData and an actor composite
  // to display the data.
  void AcceptCallback();

  // Description:
  // We need to set the original image.
  void InitializeAssignment();
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetPVOutput(vtkPVImageData *pvi);
  vtkPVImageData *GetPVOutput();

protected:
  vtkPVImageSource();
  ~vtkPVImageSource() {};
  vtkPVImageSource(const vtkPVImageSource&) {};
  void operator=(const vtkPVImageSource&) {};
  
  // Description:
  // Cast to the correct type.
  vtkImageSource *GetVTKImageSource();
};

#endif
