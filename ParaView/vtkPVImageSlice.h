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
#include "vtkKWLabeledEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWRadioButton.h"
#include "vtkPVSource.h"
#include "vtkInteractorStyleImageExtent.h"

class vtkPVImage;

class VTK_EXPORT vtkPVImageSlice : public vtkPVSource
{
public:
  static vtkPVImageSlice* New();
  vtkTypeMacro(vtkPVImageSlice, vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();
  
  // Description:
  // The methods executes on all processes.
  void SetInput(vtkPVImage *pvData);
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.
  void SetOutput(vtkPVImage *pvd);
  vtkPVImage *GetOutput();
  
  // Description:
  // This is called when the accept button is pressed.
  // The first time, it creates data.
  void SliceChanged();

  // Description:
  // Manual method for selecting which axis gets sliced.
  void SetSliceAxis(int axis);

  // Description:
  // Manual method for selecting which slice to display
  void SetSliceNumber(int slice);

  // Description:
  // Callbacks from the Select SliceAxis buttons.
  // This is the only way to get radio button fuctionality.
  void SelectXCallback();
  void SelectYCallback();
  void SelectZCallback();
  
  // Description:
  // This is the parallel method.  SetSliceAxis ... should really be parallel,
  // but this is easier since the accept method makes this call.
  // This call should not be called externally because the
  // output whole extent is computed from the SliceAxis ...
  void SetOutputWholeExtent(int xmin, int xmax, int ymin, int ymax,
			    int zmin, int zmax);

  // Description:
  // Get the underlying vtkImageSlice
  vtkGetObjectMacro(Slice, vtkImageClip);
  
  vtkGetObjectMacro(SliceStyle, vtkInteractorStyleImageExtent);
  
  // Description:
  // overriding the Select/Deselect methods in the superclass so that we can
  // add / remove the slice style button from the toolbar when this filter is
  // selected / deselected
  virtual void Select(vtkKWView *view);
  virtual void Deselect(vtkKWView *view);
  
  void UseSliceStyle();
  vtkGetObjectMacro(SliceEntry, vtkKWLabeledEntry);
  
protected:
  vtkPVImageSlice();
  ~vtkPVImageSlice();
  vtkPVImageSlice(const vtkPVImageSlice&) {};
  void operator=(const vtkPVImageSlice&) {};
  
  // This is called to update the widgets when the parameters are changed
  // programatically (not through UI).
  void UpdateProperties();
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *SourceButton;
  vtkKWLabeledEntry *SliceEntry;
  vtkKWRadioButton *XDimension;
  vtkKWRadioButton *YDimension;
  vtkKWRadioButton *ZDimension;

  // Keep the parameters here (in addition to the widgets)
  // so they can be set before the properties are created.
  int PropertiesCreated;
  int SliceNumber;
  int SliceAxis;
  
  vtkImageClip  *Slice;
  vtkInteractorStyleImageExtent *SliceStyle;
  vtkKWPushButton *SliceStyleButton;
  int SliceStyleCreated;
};

#endif
