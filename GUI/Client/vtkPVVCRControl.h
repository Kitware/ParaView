/*=========================================================================

  Program:   ParaView
  Module:    vtkPVVCRControl.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVVCRControl - Toolbar for the VCR control.
// .SECTION Description
// Toolbar for the vcr buttons.

#ifndef __vtkPVVCRControl_h
#define __vtkPVVCRControl_h

#include "vtkKWToolbar.h"

class vtkKWPushButton;
class vtkKWCheckButton;

class VTK_EXPORT vtkPVVCRControl : public vtkKWToolbar
{
public:
  static vtkPVVCRControl* New();
  vtkTypeRevisionMacro(vtkPVVCRControl, vtkKWToolbar);
  void PrintSelf(ostream& os, vtkIndent indent);
 
  void SetPlayCommand(vtkKWObject* calledObject, const char* commandString);
  void SetStopCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToBeginningCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToEndCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToPreviousCommand(vtkKWObject* calledObject, const char* commandString);
  void SetGoToNextCommand(vtkKWObject* calledObject, const char* commandString);
  void SetLoopCheckCommand(vtkKWObject* calledObject, const char* commandString);
  void SetRecordCheckCommand(vtkKWObject* calledObject, const char* commandString);
  void SetRecordStateCommand(vtkKWObject* calledObject, const char* commandString);

  virtual void Create(vtkKWApplication* app);
  virtual void UpdateEnableState();

  vtkSetMacro(InPlay, int);
  vtkGetMacro(InPlay, int);

  void SetLoopButtonState(int state);
  int GetLoopButtonState();

  void SetRecordCheckButtonState(int state);
  int GetRecordCheckButtonState();
    
  void PlayCallback();
  void StopCallback();
  void GoToBeginningCallback();
  void GoToEndCallback();
  void GoToPreviousCallback();
  void GoToNextCallback();
  void LoopCheckCallback();
  void RecordCheckCallback();
  void RecordStateCallback();

protected:
  vtkPVVCRControl();
  ~vtkPVVCRControl();

  int InPlay; // used to decide enable state of the buttons.
  vtkKWPushButton *PlayButton;
  vtkKWPushButton *StopButton;
  vtkKWPushButton *GoToBeginningButton;
  vtkKWPushButton *GoToEndButton;
  vtkKWPushButton *GoToPreviousButton;
  vtkKWPushButton *GoToNextButton;
  vtkKWCheckButton *LoopCheckButton;
  vtkKWCheckButton *RecordCheckButton;
  vtkKWPushButton *RecordStateButton;

  char* PlayCommand;
  char* StopCommand;
  char* GoToBeginningCommand;
  char* GoToEndCommand;
  char* GoToPreviousCommand;
  char* GoToNextCommand;
  char* LoopCheckCommand;
  char* RecordCheckCommand;
  char* RecordStateCommand;

  vtkSetStringMacro(PlayCommand);
  vtkSetStringMacro(StopCommand);
  vtkSetStringMacro(GoToBeginningCommand);
  vtkSetStringMacro(GoToEndCommand);
  vtkSetStringMacro(GoToPreviousCommand);
  vtkSetStringMacro(GoToNextCommand);
  vtkSetStringMacro(LoopCheckCommand);
  vtkSetStringMacro(RecordCheckCommand);
  vtkSetStringMacro(RecordStateCommand);

  void InvokeCommand(const char* command);
private:
  vtkPVVCRControl(const vtkPVVCRControl&); // Not implemented.
  void operator=(const vtkPVVCRControl&); // Not implemented.
};

#endif

