/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWNotebook.h
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
// .NAME vtkKWNotebook - a tabbed notebook of UI pages
// .SECTION Description
// The notebook represents a tabbed notebook component where you can
// add or remove pages.

#ifndef __vtkKWNotebook_h
#define __vtkKWNotebook_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWNotebook : public vtkKWWidget
{
public:
  static vtkKWNotebook* New();
  vtkTypeMacro(vtkKWNotebook,vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Raise the specified tab to be on the top.
  virtual void Raise(int n);
  void Raise(const char *name);
  void RaiseCurrent(){this->Raise(this->Current);};

  // Description:
  // Some callback routines to handle rezise events.
  void ScheduleResize();
  void Resize();
  
  // Description:
  // Get the vtkKWWidget for the frame of the specified page (Tab).
  // The UI components should be inserted into these frames.
  vtkKWWidget *GetFrame(int n);
  vtkKWWidget *GetFrame(const char *name);

  // Description:
  // Set/Get the number of pages in the notebook.
  void AddPage(const char *title);
  vtkGetMacro(NumberOfPages,int);
  
  // Description:
  // The notebook will automatically resize itself to fit its
  // contents. This can lead to a lot of resizing. So you can
  // specify a minimum width and height for the notebook. This
  // can be used to significantly reduce or eliminate the resizing
  // of the notebook.
  vtkSetMacro(MinimumWidth,int);
  vtkSetMacro(MinimumHeight,int);
  vtkGetMacro(MinimumWidth,int);
  vtkGetMacro(MinimumHeight,int);
  
protected:
  vtkKWNotebook();
  ~vtkKWNotebook();
  vtkKWNotebook(const vtkKWNotebook&) {};
  void operator=(const vtkKWNotebook&) {};

  int MinimumWidth;
  int MinimumHeight;
  int Current;
  int Height;
  int Pad;
  int BorderWidth;
  int Expanding;
  
  vtkKWWidget *TabsFrame;
  vtkKWWidget *Body;
  vtkKWWidget *Mask;
  vtkKWWidget *MaskLeft;
  vtkKWWidget *MaskLeft2;
  vtkKWWidget *MaskRight;
  vtkKWWidget *MaskRight2;
  int NumberOfPages;
  vtkKWWidget **Frames;
  vtkKWWidget **Buttons;
  char **Titles;
};


#endif


