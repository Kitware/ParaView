/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWMessageDialog.h
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
// .NAME vtkKWMessageDialog - a message dialog superclass
// .SECTION Description
// A generic superclass for MessageDialog boxes.

#ifndef __vtkKWMessageDialog_h
#define __vtkKWMessageDialog_h

#include "vtkKWDialog.h"
class vtkKWApplication;
class vtkKWImageLabel;

class VTK_EXPORT vtkKWMessageDialog : public vtkKWDialog
{
public:
  static vtkKWMessageDialog* New();
  vtkTypeMacro(vtkKWMessageDialog,vtkKWDialog);

  //BTX
  enum {Message = 0,
        YesNo,
        OkCancel};
  
  enum {NoIcon = 0,
	Error,
	Warning,
	Question,
	Info};

  enum {NoneDefault = 0,
	YesDefault = 1,
	NoDefault = 2};
	
  //ETX
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the text of the message
  void SetText(const char *);

  // Description:
  // Set the icon in the message dialog
  void SetIcon(int);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();

  // Description:
  // Set the default button
  vtkSetClampMacro(Default, int, 0, 2);
  vtkGetMacro(Default, int );

  // Description:
  // Set the style of the message box
  vtkSetMacro(Style,int);
  vtkGetMacro(Style,int);
  void SetStyleToMessage() {this->SetStyle(vtkKWMessageDialog::Message);};
  void SetStyleToYesNo() {this->SetStyle(vtkKWMessageDialog::YesNo);};
  void SetStyleToOkCancel() {this->SetStyle(vtkKWMessageDialog::OkCancel);};

  // Description:
  // Utility methods to create various dialog windows.
  static void PopupMessage(vtkKWApplication *app, vtkKWWindow *masterWin,
			   unsigned int icon, const char* title, 
			   const char*message);
  static int PopupYesNo(vtkKWApplication *app,  vtkKWWindow *masterWin, 
			unsigned int icon, const char* title, 
			const char*message);
  static int PopupOkCancel(vtkKWApplication *app, vtkKWWindow *masterWin,
			   unsigned int icon, const char* title, 
			   const char*message);

protected:
  vtkKWMessageDialog();
  ~vtkKWMessageDialog();
  vtkKWMessageDialog(const vtkKWMessageDialog&) {};
  void operator=(const vtkKWMessageDialog&) {};

  int Style;
  int Default;

  vtkKWWidget *MessageDialogFrame;
  vtkKWWidget *Label;
  vtkKWWidget *ButtonFrame;
  vtkKWWidget *OKButton;
  vtkKWWidget *CancelButton;  
  vtkKWImageLabel *Icon;
  vtkKWWidget *OKFrame;
  vtkKWWidget *CancelFrame;
};


#endif


