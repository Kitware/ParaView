/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWSplitFrame.h
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


