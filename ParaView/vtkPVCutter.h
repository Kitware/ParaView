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

#include "vtkCutter.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkPVSource.h"
#include "vtkInteractorStylePlane.h"

class vtkPVPolyData;
class vtkPVImageData;


class VTK_EXPORT vtkPVCutter : public vtkPVSource
{
public:
  static vtkPVCutter* New();
  vtkTypeMacro(vtkPVCutter, vtkPVSource);

  // Description:
  // You have to clone this object before you create its UI.
  void CreateProperties();
  
  // Description:
  // The methods executes on all processes.
  void SetInput(vtkPVData *pvData);
  
  // Description:
  // For now you have to set the output explicitly.  This allows you to manage
  // the object creation/tcl-names in the other processes.  Do not try to
  // set the output before the input has been set.
  void SetPVOutput(vtkPVPolyData *pvd);
  vtkPVPolyData *GetPVOutput();
  
  // Description:
  // This interface broadcasts the change to all the processes.
  void SetCutPlane(float originX, float originY, float originZ,
		   float normalX, float normalY, float normalZ);
  
  void CutterChanged();
  void GetSource();

  vtkGetObjectMacro(Cutter, vtkCutter);
  vtkGetObjectMacro(PlaneStyle, vtkInteractorStylePlane);
  
  void UsePlaneStyle();

  // Description:
  // Need to be able to get these to be able to change their values from a
  // callback.
  vtkGetObjectMacro(OriginXEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(OriginYEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(OriginZEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(NormalXEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(NormalYEntry, vtkKWLabeledEntry);
  vtkGetObjectMacro(NormalZEntry, vtkKWLabeledEntry);
  
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
  
  vtkKWPushButton *Accept;
  vtkKWPushButton *SourceButton;

  vtkKWLabeledFrame *OriginFrame;
  vtkKWLabeledFrame *NormalFrame;
  vtkKWLabeledEntry *OriginXEntry;
  vtkKWLabeledEntry *OriginYEntry;
  vtkKWLabeledEntry *OriginZEntry;
  vtkKWLabeledEntry *NormalXEntry;
  vtkKWLabeledEntry *NormalYEntry;
  vtkKWLabeledEntry *NormalZEntry;
  vtkKWCheckButton *ShowCrosshairButton;

  vtkKWPushButton *PlaneStyleButton;
  
  vtkCutter  *Cutter;
  vtkInteractorStylePlane *PlaneStyle;

  int PlaneStyleCreated;
  int PlaneStyleUsed;
};

#endif
