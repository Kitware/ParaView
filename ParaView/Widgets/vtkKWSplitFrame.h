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
  void PrintSelf(ostream& os, vtkIndent indent);

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
  vtkGetMacro(Frame1MinimumSize, int);
  vtkGetMacro(Frame2MinimumSize, int);
  void SetFrame1MinimumSize(int minSize);
  void SetFrame2MinimumSize(int minSize);

  // Description:
  // Set/Get the width of the left frame.  
  // For now, we cannot direclty set the width of the second frame.
  vtkGetMacro(Frame1Size, int);
  vtkGetMacro(Frame2Size, int);
  void SetFrame1Size(int minSize);

  // Description:
  // This sets the separators narrow dimension. Horizontal=> size = width.  
  // If the size is 0, then the two frames cannot be adjusted by the user.
  void SetSeparatorSize(int size);
  vtkGetMacro(SeparatorSize, int);

  // Callbacks used internally to adjust the widths,
  void DragCallback();
  void ConfigureCallback();

  // Description:
  // Vertical orientation has the first frame on top of the second frame.
  // Horizontal orientation has first frame to left of second.
  // At the moment, this state has to be set before the
  // widget is created.  I should extend it in the future to be more flexible.
  vtkSetMacro(Orientation, int);
  vtkGetMacro(Orientation, int);
  void SetOrientationToHorizontal();
  void SetOrientationToVertical();

//BTX
  enum OrientationStates {
    Horizontal = 0,
    Vertical
  };
//ETX

protected:
  vtkKWSplitFrame();
  ~vtkKWSplitFrame();

  vtkKWWidget *Frame1;
  vtkKWWidget *Separator;
  vtkKWWidget *Frame2;

  int Size;
  int Frame1Size;
  int Frame2Size;
  int SeparatorSize;

  int Frame1MinimumSize;
  int Frame2MinimumSize;

  int Orientation;

  // Reset the actual windows to match our width IVars.
  void Update();
private:
  vtkKWSplitFrame(const vtkKWSplitFrame&); // Not implemented
  void operator=(const vtkKWSplitFrame&); // Not implemented
};


#endif


