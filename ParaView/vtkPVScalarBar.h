/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVScalarBar.h
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
// .NAME vtkPVScalarBar - Class to add scalar bars into ParaView
// .SECTION Description
// Create a scalar bar to be associated with each data object.


#ifndef __vtkPVScalarBar_h
#define __vtkPVScalarBar_h

#include "vtkKWComposite.h"
#include "vtkScalarBarActor.h"
#include "vtkPVData.h"
#include "vtkKWPushButton.h"
#include "vtkKWMenuButton.h"
#include "vtkKWLabeledEntry.h"
#include "vtkKWCheckButton.h"
#include "vtkKWScale.h"

class VTK_EXPORT vtkPVScalarBar : public vtkKWComposite
{
public:
  static vtkPVScalarBar* New();
  vtkTypeMacro(vtkPVScalarBar, vtkKWComposite);

  // Description:
  // Get the scalar bar.  (need to override pure virtual function in
  // vtkKWComposite)
  vtkProp *GetProp();

  // Description:
  // Create/Show the UI.
  void CreateProperties();

  // Description:
  // Set/Get the underlying vtkScalarBarActor.
  vtkSetObjectMacro(ScalarBar, vtkScalarBarActor);
  vtkGetObjectMacro(ScalarBar, vtkScalarBarActor);
  
  // Description:
  // ONLY SET THIS IF YOU ARE A PVDATA!
  // The scalar bar needs to know who owns it.
  void SetPVData(vtkPVData *data);
  vtkGetObjectMacro(PVData, vtkPVData);
  
  // Description:
  // Method called by the push button to return to the data notebook.
  void ShowDataNotebook();

  void SetOrientationToHorizontal();
  void SetOrientationToVertical();

  // Description:
  // This flag turns the visibility of the prop on and off.
  void SetVisibility(int v);
  int GetVisibility();
  vtkBooleanMacro(Visibility, int);
  
  // Description:
  // This method is a callback from the checkbutton that controls the
  // visibility of the PVScalarBar.
  void SetScalarBarVisibility();
  
  // Description:
  // This method is a callback from the entry box that controls the
  // title of the PVScalarBar.
  void SetScalarBarTitle();

  // Description:
  // These methods are callbacks from the sliders that control the
  // height, width, X position, and Y position of the PVScalarBar.
  void SetScalarBarHeight();
  void SetScalarBarWidth();
  void SetScalarBarXPosition();
  void SetScalarBarYPosition();

  void UpdateLookupTable();
  
protected:
  vtkPVScalarBar();
  ~vtkPVScalarBar();
  vtkPVScalarBar(const vtkPVScalarBar&) {};
  void operator=(const vtkPVScalarBar&) {};
  
  int Visibility;
  
  vtkScalarBarActor *ScalarBar;
  vtkKWPushButton *DataNotebookButton;
  vtkKWMenuButton *OrientationMenu;
  vtkKWCheckButton *VisibilityButton;
  vtkKWLabeledEntry *TitleEntry;
  vtkKWScale *WidthScale;
  vtkKWScale *HeightScale;
  vtkKWScale *XPositionScale;
  vtkKWScale *YPositionScale;
  vtkKWWidget *Properties;
  
  // The vtkPVData object that owns this vtkPVScalarBar object.
  vtkPVData *PVData;
};

#endif
