/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWPushButton.h
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
// .NAME vtkKWPushButton - check button widget
// .SECTION Description
// A simple widget that represents a check button. It can be modified 
// and queried using the GetState and SetState methods.

#ifndef __vtkKWPushButton_h
#define __vtkKWPushButton_h

#include "vtkKWWidget.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWPushButton : public vtkKWWidget
{
public:
  static vtkKWPushButton* New();
  vtkTypeMacro(vtkKWPushButton,vtkKWWidget);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, char *args);

protected:
  vtkKWPushButton() {};
  ~vtkKWPushButton() {};
  vtkKWPushButton(const vtkKWPushButton&) {};
  void operator=(const vtkKWPushButton&) {};
  
};


#endif


