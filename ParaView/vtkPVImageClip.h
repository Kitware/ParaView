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

#include "vtkKWPushButton.h"
#include "vtkImageClip.h"
#include "vtkKWLabeledEntry.h"
#include "vtkPVSource.h"
#include "vtkInteractorStyleImageExtent.h"

class vtkPVImageData;



class VTK_EXPORT vtkPVImageClip : public vtkPVSource
{
public:
  static vtkPVImageClip* New();
  vtkTypeMacro(vtkPVImageClip, vtkPVSource);

  // Description:
  // need to override the superclass's Select/Deselect methods so we can pack/
  // pack forget the image extent button when this PVSource is deselected
  virtual void Select(vtkKWView *view);
  virtual void Deselect(vtkKWView *view);  

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();

  // Description:
  // Used to connect pipelines.  Executes in all processes.
  void SetInput(vtkPVImageData *pvi);

  vtkPVImageData *GetInput();
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetPVOutput(vtkPVImageData *pvd);
  vtkPVImageData *GetPVOutput();

  // Description:
  // Changes filters clip and clones in satellite processes.
  void SetOutputWholeExtent(int xMin, int xMax, int yMin, int yMax, int zMin, int zMax);
  
  void ExtentsChanged();
  void GetSource();

  vtkGetObjectMacro(ImageClip, vtkImageClip);
  vtkGetObjectMacro(ExtentStyle, vtkInteractorStyleImageExtent);

  // Description:
  // need to be able to access the entry widgets from the interactor style's
  // callback
  vtkGetObjectMacro(ClipXMinEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(ClipXMaxEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(ClipYMinEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(ClipYMaxEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(ClipZMinEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(ClipZMaxEntry, vtkKWLabeledEntry);
  
  void UseExtentStyle();
  
protected:
  vtkPVImageClip();
  ~vtkPVImageClip();
  vtkPVImageClip(const vtkPVImageClip&) {};
  void operator=(const vtkPVImageClip&) {};
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *SourceButton;
  vtkKWLabeledEntry *ClipXMinEntry;
  vtkKWLabeledEntry *ClipXMaxEntry;
  vtkKWLabeledEntry *ClipYMinEntry;
  vtkKWLabeledEntry *ClipYMaxEntry;
  vtkKWLabeledEntry *ClipZMinEntry;
  vtkKWLabeledEntry *ClipZMaxEntry;

  vtkKWPushButton *ExtentStyleButton;
  
  vtkImageClip  *ImageClip;
  vtkInteractorStyleImageExtent *ExtentStyle;
  int ExtentStyleCreated;
};

#endif
