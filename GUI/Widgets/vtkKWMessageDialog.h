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
class vtkKWMessage;
class vtkKWLabel;
class vtkKWPushButton;

class KWWidgets_EXPORT vtkKWMessageDialog : public vtkKWDialog
{
public:
  static vtkKWMessageDialog* New();
  vtkTypeRevisionMacro(vtkKWMessageDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create();

  // Description:
  // Set the text of the message (and the width of a line, in pixels)
  virtual void SetText(const char *);
  virtual void SetTextWidth(int);
  virtual int GetTextWidth();

  // Description:
  // Status of the dialog. This subclass defines a new 'Other' status on
  // top of the usual one (active e.g. displayed, canceled, OK'ed). This
  // status is triggered by pressing the 'Other' button.
  //BTX
  enum 
  {
    StatusOther = 100
  };
  //ETX

  // Description:
  // Set the style of the message box.
  // No effect if called after Create()
  //BTX
  enum 
  {
    StyleMessage = 0,
    StyleYesNo,
    StyleOkCancel,
    StyleOkOtherCancel,
    StyleCancel
  };
  //ETX
  vtkSetMacro(Style,int);
  vtkGetMacro(Style,int);
  void SetStyleToMessage() 
    { this->SetStyle(vtkKWMessageDialog::StyleMessage); };
  void SetStyleToYesNo() 
    { this->SetStyle(vtkKWMessageDialog::StyleYesNo); };
  void SetStyleToOkCancel() 
    { this->SetStyle(vtkKWMessageDialog::StyleOkCancel); };
  void SetStyleToOkOtherCancel() 
    { this->SetStyle(vtkKWMessageDialog::StyleOkOtherCancel); };
  void SetStyleToCancel() 
    { this->SetStyle(vtkKWMessageDialog::StyleCancel); };

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
    InvokeAtPointer = 0x01000
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
  static void PopupMessage(vtkKWApplication *app, vtkKWWindowBase *masterWin,
                           const char* title, 
                           const char* message, int options = 0);
  static int PopupYesNo(vtkKWApplication *app,  vtkKWWindowBase *masterWin,
                        const char* title, 
                        const char* message, int options = 0);
  static int PopupYesNo(vtkKWApplication *app,  vtkKWWindowBase *masterWin, 
                        const char* name, 
                        const char* title, const char* message, 
                        int options = 0);
  static int PopupOkCancel(vtkKWApplication *app, vtkKWWindowBase *masterWin,
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

  // Description:
  // Set or get the message dialog name. This name is use to save/restore
  // information about this specific dialog in the registry (for example,
  // bypass the dialog altogether by clicking on a specific button 
  // automatically). 
  // This should not be confused with
  // the message dialog title that can be set using the superclass
  // SetTitle() method. 
  vtkSetStringMacro(DialogName);
  vtkGetStringMacro(DialogName);

  // Description:
  // Convenience static method to store/retrieve a message dialog response
  // for a given application in/from the registry.
  // This can be used to prevent the user from answering the same question
  // again and again (for ex: "Are you sure you want to exit the application").
  // 'dialogname' is the name of a dialog (most likely its DialogName ivar).
  // The 'response' is arbitrary but most likely the value returned by a
  // call to Invoke() on the dialog.
  static int RestoreMessageDialogResponseFromRegistry(
    vtkKWApplication *app, const char *dialogname);
  static void SaveMessageDialogResponseToRegistry(
    vtkKWApplication *app, const char *dialogname, int response);

  // Description:
  // Dialog can be also used by performing individual steps of Invoke. These
  // steps are initialize: PreInvoke(), finalize: PostInvoke(), and check if
  // user responded IsUserDoneWithDialog(). Use this method only if you
  // want to bypass the event loop used in Invoke() by creating your own
  // and checking for IsUserDoneWithDialog().
  virtual int PreInvoke();
  virtual void PostInvoke();

  // Description::
  // Callback. Close this Dialog (for the third button)
  virtual void Other();

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
  vtkKWMessage     *Message;
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
