/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVMenuButton.h
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

#ifndef __vtkPVMenuButton_h
#define __vtkPVMenuButton_h

#include "vtkKWMenu.h"

class VTK_EXPORT vtkPVMenuButton : public vtkKWWidget
{
public:
  static vtkPVMenuButton* New();
  vtkTypeMacro(vtkPVMenuButton, vtkKWWidget);

  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, char *args);
  
  // Description:
  // Add text to the button
  void SetButtonText(char *text);
  
  // Description: 
  // Append a standard menu item and command to the current menu.
  void AddCommand(const char* label, vtkKWObject* Object,
		  const char* MethodAndArgString , const char* help = 0);
  
  vtkKWMenu* GetMenu();
  
protected:
  vtkPVMenuButton();
  ~vtkPVMenuButton();
  vtkPVMenuButton(const vtkPVMenuButton&) {};
  void operator=(const vtkPVMenuButton&) {};
  
  vtkKWMenu *Menu;
};

#endif
