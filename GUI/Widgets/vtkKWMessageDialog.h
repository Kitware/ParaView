/*=========================================================================

  Module:    vtkKWMessageDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMessageDialog - a message dialog superclass
// .SECTION Description
// A generic superclass for MessageDialog boxes.

#ifndef __vtkKWMessageDialog_h
#define __vtkKWMessageDialog_h

#include "vtkKWDialog.h"

class vtkKWApplication;
class vtkKWCheckButton;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWPushButton;

class VTK_EXPORT vtkKWMessageDialog : public vtkKWDialog
{
public:
  static vtkKWMessageDialog* New();
  vtkTypeRevisionMacro(vtkKWMessageDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Set the text of the message (and the width of a line, in pixels)
  virtual void SetText(const char *);
  virtual void SetTextWidth(int);
  virtual int GetTextWidth();

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise.
  virtual int Invoke();

  // Description:
  // Set the style of the message box
  //BTX
  enum 
  {
    Message = 0,
    YesNo,
    OkCancel,
    OkOtherCancel
  };
  //ETX
  vtkSetMacro(Style,int);
  vtkGetMacro(Style,int);
  void SetStyleToMessage() {this->SetStyle(vtkKWMessageDialog::Message);};
  void SetStyleToYesNo() {this->SetStyle(vtkKWMessageDialog::YesNo);};
  void SetStyleToOkCancel() {this->SetStyle(vtkKWMessageDialog::OkCancel);};
  void SetStyleToOkOtherCancel() 
    {this->SetStyle(vtkKWMessageDialog::OkOtherCancel);};

  // Description:
  // Set or get the message dialog name
  vtkSetStringMacro(DialogName);
  vtkGetStringMacro(DialogName);

  // Description:
  // Set different options for the dialog.
  //BTX
  enum 
  {
    RememberYes     = 0x00002,
    RememberNo      = 0x00004,
    ErrorIcon       = 0x00008,
    WarningIcon     = 0x00010,
    QuestionIcon    = 0x00020,
    YesDefault      = 0x00040,
    NoDefault       = 0x00080,
    OkDefault       = 0x00100,
    CancelDefault   = 0x00200,
    Beep            = 0x00400,
    PackVertically  = 0x00800,
    InvokeAtPointer = 0x01000,
    NoDecoration    = 0x02000
  };
  //ETX
  vtkSetMacro(Options, int);
  vtkGetMacro(Options, int);

  // Description:
  // The label displayed on the OK button. Only used when
  // the style is OkCancel.
  vtkSetStringMacro(OKButtonText);
  vtkGetStringMacro(OKButtonText);

  // Description:
  // The label displayed on the cancel button. Only used when
  // the style is OkCancel.
  vtkSetStringMacro(CancelButtonText);
  vtkGetStringMacro(CancelButtonText);

  // Description:
  // The label displayed on the other button. Only used when
  // the style is OkOtherCancel.
  vtkSetStringMacro(OtherButtonText);
  vtkGetStringMacro(OtherButtonText);

  // Description:
  // Utility methods to create various dialog windows.
  // icon is a enumerated icon type described in vtkKWIcon.
  // title is a title string of the dialog. name is the dialog name
  // used for the registry. message is the text message displayed
  // in the dialog.
  static void PopupMessage(vtkKWApplication *app, vtkKWWindow *masterWin,
                           const char* title, 
                           const char* message, int options = 0);
  static int PopupYesNo(vtkKWApplication *app,  vtkKWWindow *masterWin,
                        const char* title, 
                        const char* message, int options = 0);
  static int PopupYesNo(vtkKWApplication *app,  vtkKWWindow *masterWin, 
                        const char* name, 
                        const char* title, const char* message, 
                        int options = 0);
  static int PopupOkCancel(vtkKWApplication *app, vtkKWWindow *masterWin,
                           const char* title, 
                           const char* message, int options = 0);

  // Description:
  // Retrieve the frame where the message is.
  vtkGetObjectMacro(TopFrame, vtkKWFrame);
  vtkGetObjectMacro(MessageDialogFrame, vtkKWFrame);
  vtkGetObjectMacro(BottomFrame, vtkKWFrame);

  // Description:
  // Set the icon on the message dialog.
  virtual void SetIcon();

  // Description:
  // Accessor for OK and cancel button
  vtkGetObjectMacro(OKButton, vtkKWPushButton);
  vtkGetObjectMacro(CancelButton, vtkKWPushButton);
  vtkGetObjectMacro(OtherButton, vtkKWPushButton);

  // Description::
  // Close this Dialog (for the third button)
  virtual void Other();

  // Description:
  // Convenience method to guess the width/height of the dialog.
  virtual int GetWidth();
  virtual int GetHeight();

protected:
  vtkKWMessageDialog();
  ~vtkKWMessageDialog();

  int             Style;
  int             Default;
  int             Options;
  char            *DialogName;
  char            *DialogText;

  vtkSetStringMacro(DialogText);
  vtkGetStringMacro(DialogText);

  vtkKWFrame       *TopFrame;
  vtkKWFrame       *MessageDialogFrame;
  vtkKWFrame       *BottomFrame;
  vtkKWLabel       *Label;
  vtkKWFrame       *ButtonFrame;
  vtkKWPushButton  *OKButton;
  vtkKWPushButton  *CancelButton;  
  vtkKWPushButton  *OtherButton;  
  vtkKWLabel       *Icon;
  vtkKWFrame       *OKFrame;
  vtkKWFrame       *CancelFrame;
  vtkKWFrame       *OtherFrame;
  vtkKWCheckButton *CheckButton;

  // Description:
  // Get the value of the check box for remembering the answer from
  // the user.
  int GetRememberMessage();
  
  char* OKButtonText;
  char* CancelButtonText;
  char* OtherButtonText;

private:
  vtkKWMessageDialog(const vtkKWMessageDialog&); // Not implemented
  void operator=(const vtkKWMessageDialog&); // Not implemented
};


#endif
