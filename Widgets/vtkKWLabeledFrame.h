/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWLabeledFrame.h
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

protected:
  vtkKWLabeledFrame();
  ~vtkKWLabeledFrame();
  vtkKWLabeledFrame(const vtkKWLabeledFrame&) {};
  void operator=(const vtkKWLabeledFrame&) {};

  vtkKWWidget *Border;
  vtkKWWidget *Border2;
  vtkKWWidget *Frame;
  vtkKWWidget *Groove;
  vtkKWWidget *Label;
};


#endif


