/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWProgressGauge.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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
