/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWToolbar.h
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
// .NAME vtkKWToolbar
// .SECTION Description
// Simply a frame to hold a bunch of tools.  It uses bindings to control
// the height of the frame.
// In the future we could use the object to move toolbars groups around.

#ifndef __vtkKWToolbar_h
#define __vtkKWToolbar_h

#include "vtkKWWidget.h"
class vtkKWApplication;


class VTK_EXPORT vtkKWToolbar : public vtkKWWidget
{
public:
  vtkKWToolbar();
  ~vtkKWToolbar();
  vtkKWToolbar(const vtkKWToolbar&) {};
  void operator=(const vtkKWToolbar&) {};
  static vtkKWToolbar* New() {return new vtkKWToolbar;};
  const char *GetClassName() {return "vtkKWToolbar";};

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);

  // Description:
  // All toolbars should be the same height.
  vtkSetMacro(Height, int);
  vtkGetMacro(Height, int);

  // Description:
  // Callbacks to ensure height stays the same.
  void ScheduleResize();
  void Resize();


protected:

  // Height stuf is not working (ask ken)
  int Height;
  int Expanding;

  vtkKWWidget *Bar1;
  vtkKWWidget *Bar2;

};


#endif


