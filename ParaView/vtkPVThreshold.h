/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVThreshold.h
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
// .NAME vtkPVThreshold - A class to handle the UI for vtkThreshold
// .SECTION Description


#ifndef __vtkPVThreshold_h
#define __vtkPVThreshold_h

#include "vtkPVSource.h"
#include "vtkKWPushButton.h"
#include "vtkKWLabeledFrame.h"
#include "vtkKWMenuButton.h"
#include "vtkKWOptionMenu.h"
#include "vtkKWScale.h"
#include "vtkKWCheckButton.h"
#include "vtkThreshold.h"

class VTK_EXPORT vtkPVThreshold : public vtkPVSource
{
public:
  static vtkPVThreshold* New();
  vtkTypeMacro(vtkPVThreshold, vtkPVSource);
    
  // Description:
  // Set up the UI for this source
  void CreateProperties();

  // Description:
  // Tcl callback for the attribute mode option menu
  void ChangeAttributeMode(const char* newMode);

  // Description:
  // Tcl callbacks for the upper value and lower value scales
  void UpperValueCallback();
  void LowerValueCallback();

  // Description:
  // Save this source to a file.
  void Save(ofstream *file);
  
protected:
  vtkPVThreshold();
  ~vtkPVThreshold();
  vtkPVThreshold(const vtkPVThreshold&) {};
  void operator=(const vtkPVThreshold&) {};

  vtkThreshold* Threshold;
  
  vtkKWWidget* AttributeModeFrame;
  vtkKWLabel* AttributeModeLabel;
  vtkKWOptionMenu* AttributeModeMenu;
  vtkKWScale* UpperValueScale;
  vtkKWScale* LowerValueScale;
  vtkKWCheckButton* AllScalarsCheck;
};

#endif
