/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVImageClip.h
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
#ifndef __vtkPVImageClip_h
#define __vtkPVImageClip_h

#include "vtkKWLabel.h"
#include "vtkImageClip.h"
#include "vtkKWEntry.h"
#include "vtkPVSource.h"

class vtkPVImage;



class VTK_EXPORT vtkPVImageClip : public vtkPVSource
{
public:
  static vtkPVImageClip* New();
  vtkTypeMacro(vtkPVImageClip, vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  int Create(char *args);

  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetOutput(vtkPVImage *pvd);
  vtkPVImage *GetOutput();
  
  void ExtentsChanged();

  vtkGetObjectMacro(ImageClip, vtkImageClip);
  
protected:
  vtkPVImageClip();
  ~vtkPVImageClip();
  vtkPVImageClip(const vtkPVImageClip&) {};
  void operator=(const vtkPVImageClip&) {};
  
  vtkKWWidget *Accept;
  vtkKWEntry *ClipXMinEntry;
  vtkKWLabel *ClipXMinLabel;
  vtkKWEntry *ClipXMaxEntry;
  vtkKWLabel *ClipXMaxLabel;
  vtkKWEntry *ClipYMinEntry;
  vtkKWLabel *ClipYMinLabel;
  vtkKWEntry *ClipYMaxEntry;
  vtkKWLabel *ClipYMaxLabel;
  vtkKWEntry *ClipZMinEntry;
  vtkKWLabel *ClipZMinLabel;
  vtkKWEntry *ClipZMaxEntry;
  vtkKWLabel *ClipZMaxLabel;

  vtkImageClip  *ImageClip;
};

#endif
