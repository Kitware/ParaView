/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWEntry.h
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
// .NAME vtkKWEntry - a single line text entry widget
// .SECTION Description
// A simple widget used for collecting keyboard input from the user. This
// widget provides support for single line input.

#ifndef __vtkKWEntry_h
#define __vtkKWEntry_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWEntry : public vtkKWWidget
{
public:
  static vtkKWEntry* New();
  vtkTypeMacro(vtkKWEntry,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set/Get the value of the entry in a few different formats.
  // In the SetValue method with float, the second argument is the
  // number of decimal places to display.
  void SetValue(const char *);
  void SetValue(int a);
  void SetValue(float f,int size);
  char *GetValue();
  int GetValueAsInt();
  float GetValueAsFloat();
  
  // Description:
  // The width is the number of charaters wide the entry box can fit.
  // To keep from changing behavior of the entry,  the default
  // value is -1 wich means the width is not explicitely set.
  void SetWidth(int width);
  vtkGetMacro(Width, int);

protected:
  vtkKWEntry();
  ~vtkKWEntry();
  vtkKWEntry(const vtkKWEntry&) {};
  void operator=(const vtkKWEntry&) {};
  
  vtkSetStringMacro(ValueString);
  vtkGetStringMacro(ValueString);
  
  char *ValueString;
  int Width;
  
};


#endif


