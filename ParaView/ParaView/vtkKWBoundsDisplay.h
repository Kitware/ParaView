/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWBoundsDisplay.h
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
// .NAME vtkKWBoundsDisplay - an entry with a label
// .SECTION Description
// The LabeledEntry creates an entry with a label in front of it; both are
// contained in a frame


#ifndef __vtkKWBoundsDisplay_h
#define __vtkKWBoundsDisplay_h

#include "vtkKWLabeledFrame.h"

class vtkKWApplication;
class vtkKWLabel;

class VTK_EXPORT vtkKWBoundsDisplay : public vtkKWLabeledFrame
{
public:
  static vtkKWBoundsDisplay* New();
  vtkTypeRevisionMacro(vtkKWBoundsDisplay, vtkKWLabeledFrame);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Set the bounds to display.
  void SetBounds(double bounds[6]);
  void SetExtent(int ext[6]); 
  vtkGetVector6Macro(Bounds, double);

  // Description:
  // I want to use this widget to display an extent (int values).
  // This mode causes the extent to be printed as integers.
  // This flag is set to Bounds by default.
  // The mode is automatically set when the bounds or extent is set.
  void SetModeToExtent() {this->ExtentMode = 1; this->UpdateWidgets();}
  void SetModeToBounds() {this->ExtentMode = 0; this->UpdateWidgets();}

protected:
  vtkKWBoundsDisplay();
  ~vtkKWBoundsDisplay();

  void UpdateWidgets();

  vtkKWLabel *XRangeLabel;
  vtkKWLabel *YRangeLabel;
  vtkKWLabel *ZRangeLabel;

  double Bounds[6];
  int Extent[6];
  int ExtentMode;

  vtkKWBoundsDisplay(const vtkKWBoundsDisplay&); // Not implemented
  void operator=(const vtkKWBoundsDisplay&); // Not implemented
};


#endif
