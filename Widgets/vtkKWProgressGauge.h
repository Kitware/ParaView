/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWProgressGauge.h
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
// .NAME vtkKWProgressGauge - a progress bar widget
// .SECTION Description
// A simple widget used for displaying a progress bar with a percent value
// text in the center of the widget.

#ifndef __vtkKWProgressGauge_h
#define __vtkKWProgressGauge_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWProgressGauge : public vtkKWWidget
{
public:
  static vtkKWProgressGauge* New();
  vtkTypeMacro(vtkKWProgressGauge,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, char *args);

  // Description:
  // Set the length and width of the widget
  vtkSetMacro(Length, int);
  vtkSetMacro(Height, int);
  
  // Description:
  // Set the percentage displayed.  This number is forced to be in
  // the range 0 to 100.
  void SetValue(int value);
  
  // Description:
  // Set the color of the progress bar, the default is blue.
  // Set the color of the background, the default is gray.
  vtkSetStringMacro(BarColor);
  vtkSetStringMacro(BackgroundColor);

protected:
  vtkKWProgressGauge();
  ~vtkKWProgressGauge();
  vtkKWProgressGauge(const vtkKWProgressGauge&) {};
  void operator=(const vtkKWProgressGauge&) {};
private:
  int Length;
  int Height;
  char* BarColor;
  char* BackgroundColor;
  int Value;
};


#endif
