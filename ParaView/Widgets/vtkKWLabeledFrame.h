/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledFrame.h
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
// .NAME vtkKWLabeledFrame - a frame with a grooved border and a label
// .SECTION Description
// The LabeledFrame creates a frame with a grooved border, and a label
// embedded in the upper left corner of the grooved border.


#ifndef __vtkKWLabeledFrame_h
#define __vtkKWLabeledFrame_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWLabeledFrame : public vtkKWWidget
{
public:
  static vtkKWLabeledFrame* New();
  vtkTypeMacro(vtkKWLabeledFrame,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);

  // Description:
  // Set the label for the frame.
  void SetLabel(const char *);
  
  // Description:
  // Get the vtkKWWidget for the internal frame.
  vtkKWWidget *GetFrame() {return this->Frame;};

  // Description:
  // Show or hide the frame.
  void ShowHideFrame();
  static void AllowShowHideOn();
  static void AllowShowHideOff();

protected:
  vtkKWLabeledFrame();
  ~vtkKWLabeledFrame();

  vtkKWWidget *Border;
  vtkKWWidget *Border2;
  vtkKWWidget *Frame;
  vtkKWWidget *Groove;
  vtkKWWidget *Label;
  vtkKWWidget *Icon;
  int Displayed;
  static int AllowShowHide;
private:
  vtkKWLabeledFrame(const vtkKWLabeledFrame&); // Not implemented
  void operator=(const vtkKWLabeledFrame&); // Not implemented
};


#endif


