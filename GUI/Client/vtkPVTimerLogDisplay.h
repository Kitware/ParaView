/*=========================================================================

  Program:   ParaView
  Module:    vtkPVTimerLogDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTimerLogDisplay - Shows a text version of the timer log entries.
// .SECTION Description
// A widget to display timing information in the timer log.

#ifndef __vtkPVTimerLogDisplay_h
#define __vtkPVTimerLogDisplay_h

#include "vtkKWTopLevel.h"

class vtkKWApplication;
class vtkPVApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWTextWithScrollbars;
class vtkKWFrame;
class vtkKWWindow;
class vtkKWMenuButton;
class vtkKWCheckButton;
class vtkPVTimerInformation;

class VTK_EXPORT vtkPVTimerLogDisplay : public vtkKWTopLevel
{
public:
  static vtkPVTimerLogDisplay* New();
  vtkTypeRevisionMacro(vtkPVTimerLogDisplay, vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Display the toplevel.
  virtual void Display();

  // Description:
  // This threshold eliminates the shosrt duration events fro the display.
  void SetThreshold(float val);
  vtkGetMacro(Threshold, float);

  // Description:
  // Control the maximum length of the timer log buffer.
  void SetBufferLength(int len);
  int GetBufferLength();

  // Description:
  // Saves the current log to a file.
  void Save();
  virtual void Save(const char* fileName);

  // Description:
  // Clear all entries from the buffer.
  virtual void Clear();

  // Description:
  // Call back from the EnableCheck that will stop or start loging of events.
  void EnableCheckCallback(int state);

  //BTX
  // Description:
  // Get the timer information.
  vtkPVTimerInformation* GetTimerInformation();
  //ETX

  // Description:
  // A convience method to cast KWApplication to PVApplication.
  vtkPVApplication* GetPVApplication();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Update the display copying the new log information.
  virtual void Update();
 
protected:
  vtkPVTimerLogDisplay();
  ~vtkPVTimerLogDisplay();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  void DisplayLog();

  void Append(const char*);
  
  vtkKWFrame*      ControlFrame;
  vtkKWPushButton*  SaveButton;
  vtkKWPushButton*  ClearButton;
  vtkKWPushButton*  RefreshButton;
  vtkKWLabel*       ThresholdLabel;
  vtkKWMenuButton*  ThresholdMenu;
  vtkKWLabel*       BufferLengthLabel;
  vtkKWMenuButton*  BufferLengthMenu;
  vtkKWLabel*       EnableLabel;
  vtkKWCheckButton* EnableCheck;

  vtkKWTextWithScrollbars*  DisplayText;

  vtkKWFrame*     ButtonFrame;
  vtkKWPushButton* DismissButton;
    
  float   Threshold;

  vtkPVTimerInformation* TimerInformation;

private:
  vtkPVTimerLogDisplay(const vtkPVTimerLogDisplay&); // Not implemented
  void operator=(const vtkPVTimerLogDisplay&); // Not implemented
};

#endif
