/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMessageDialog.h
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
// .NAME vtkKWMessageDialog - a message dialog superclass
// .SECTION Description
// A generic superclass for MessageDialog boxes.

#ifndef __vtkKWMessageDialog_h
#define __vtkKWMessageDialog_h

#include "vtkKWDialog.h"
class vtkKWApplication;

class VTK_EXPORT vtkKWMessageDialog : public vtkKWDialog
{
public:
  static vtkKWMessageDialog* New();
  vtkTypeMacro(vtkKWMessageDialog,vtkKWDialog);

  //BTX
  enum {Message = 0,
        YesNo,
        OkCancel};
  //ETX
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the text of the message
  void SetText(const char *);

  // Description:
  // Set the style of the message box
  vtkSetMacro(Style,int);
  vtkGetMacro(Style,int);
  void SetStyleToMessage() {this->SetStyle(vtkKWMessageDialog::Message);};
  void SetStyleToYesNo() {this->SetStyle(vtkKWMessageDialog::YesNo);};
  void SetStyleToOkCancel() {this->SetStyle(vtkKWMessageDialog::OkCancel);};
  
protected:
  vtkKWMessageDialog();
  ~vtkKWMessageDialog();
  vtkKWMessageDialog(const vtkKWMessageDialog&) {};
  void operator=(const vtkKWMessageDialog&) {};

  int Style;
  
  vtkKWWidget *Label;
  vtkKWWidget *ButtonFrame;
  vtkKWWidget *OKButton;
  vtkKWWidget *CancelButton;  
};


#endif


