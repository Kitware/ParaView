/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSplitFrame.h
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
// .NAME vtkKWSplitFrame - A Frame that contains two adjustable sub frames.
// .SECTION Description
// The split frame allows the use to select the size of the two frames.
// It uses a spearator that can be dragged interactively.


#ifndef __vtkKWSplitFrame_h
#define __vtkKWSplitFrame_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWSplitFrame : public vtkKWWidget
{
public:
  static vtkKWSplitFrame* New();
  vtkTypeMacro(vtkKWSplitFrame,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);
  
  // Description:
  // Get the vtkKWWidget for the left internal frame.
  vtkKWWidget *GetFrame1() {return this->Frame1;};

  // Description:
  // Get the vtkKWWidget for the right internal frame.
  vtkKWWidget *GetFrame2() {return this->Frame2;};

  // Description:
  // Set/Get The minimum widths for the two frames.
  // They default to 50 pixels.
  vtkGetMacro(Frame1MinimumWidth, int);
  vtkGetMacro(Frame2MinimumWidth, int);
  void SetFrame1MinimumWidth(int minWidth);
  void SetFrame2MinimumWidth(int minWidth);

  // Description:
  // Set/Get the width of the left frame.  
  // For now, we cannot direclty set the width of the second frame.
  vtkGetMacro(Frame1Width, int);
  vtkGetMacro(Frame2Width, int);
  void SetFrame1Width(int minWidth);

  // Description:
  // This sets the separator width.  if the width is 0, then
  // the two frames cannot be adjusted by the user.
  void SetSeparatorWidth(int width);
  vtkGetMacro(SeparatorWidth, int);

  // Callbacks used internally to adjust the widths,
  void DragCallback();
  void ConfigureCallback();

protected:
  vtkKWSplitFrame();
  ~vtkKWSplitFrame();
  vtkKWSplitFrame(const vtkKWSplitFrame&) {};
  void operator=(const vtkKWSplitFrame&) {};

  vtkKWWidget *Frame1;
  vtkKWWidget *Separator;
  vtkKWWidget *Frame2;

  int Width;
  int Frame1Width;
  int Frame2Width;
  int SeparatorWidth;

  int Frame1MinimumWidth;
  int Frame2MinimumWidth;

  // Reset the actual windows to match our width IVars.
  void Update();
};


#endif


