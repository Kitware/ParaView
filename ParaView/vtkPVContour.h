/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVContour.h
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
// .NAME vtkPVContour - A class to handle the UI for vtkContour
// .SECTION Description


#ifndef __vtkPVContour_h
#define __vtkPVContour_h

#include "vtkPVSource.h"
#include "vtkKWPushButton.h"
#include "vtkKWCheckButton.h"
#include "vtkKWEntry.h"
#include "vtkKWListBox.h"

class VTK_EXPORT vtkPVContour : public vtkPVSource
{
public:
  static vtkPVContour* New();
  vtkTypeMacro(vtkPVContour, vtkPVSource);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Tcl callbacks for the add/delete contour value buttons
  void AddValueCallback();
  void DeleteValueCallback();
  
  // Description:
  // Tcl callback for adding the contour values
  void ContourValuesAcceptCallback();
  void ContourValuesResetCallback();

  // Description:
  // Save this source to a file.
  void SaveInTclScript(ofstream *file);
  
protected:
  vtkPVContour();
  ~vtkPVContour();
  vtkPVContour(const vtkPVContour&) {};
  void operator=(const vtkPVContour&) {};

  vtkKWLabel* ContourValuesLabel;
  vtkKWListBox *ContourValuesList;
  vtkKWWidget* NewValueFrame;
  vtkKWLabel* NewValueLabel;
  vtkKWEntry* NewValueEntry;
  vtkKWPushButton* AddValueButton;
  vtkKWPushButton* DeleteValueButton;
  vtkKWCheckButton* ComputeNormalsCheck;
  vtkKWCheckButton* ComputeGradientsCheck;
  vtkKWCheckButton* ComputeScalarsCheck;
};

#endif
