/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWText.h
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
// .NAME vtkKWText
// .SECTION Description
// A simple widget used for collecting keyboard input from the user.

#ifndef __vtkKWText_h
#define __vtkKWText_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWText : public vtkKWWidget
{
public:
  static vtkKWText* New();
  const char *GetClassName() {return "vtkKWText";};

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

  // Description:
  // Set/Get the value of the Text.
  void SetValue(char *);
  char *GetValue();

protected:
};


#endif


