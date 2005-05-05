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

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkPVApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWText;
class vtkKWWindow;
class vtkKWOptionMenu;
class vtkKWCheckButton;
class vtkPVTimerInformation;

class VTK_EXPORT vtkPVTimerLogDisplay : public vtkKWWidget
{
public:
  static vtkPVTimerLogDisplay* New();
  vtkTypeRevisionMacro(vtkPVTimerLogDisplay, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);
  
  // Description:
  // Display the interactor
  void Display();

  // Description:
  // Callback from the dismiss button that closes the window.
  void Dismiss();

  // Description:
  // Set the title of the TclInteractor to appear in the titlebar
  vtkSetStringMacro(Title);
  
  // Description:
  // Set the window to which the dialog will be slave.
  // If set, this dialog will always be on top of the master
  // window and will minimize with it (assuming that the windowing
  // system supports this)
  void SetMasterWindow(vtkKWWindow* win);
  
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
  void EnableCheckCallback();

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

  void DisplayLog();

  void Append(const char*);
  
  vtkKWWindow*      MasterWindow;

  vtkKWWidget*      ControlFrame;
  vtkKWPushButton*  SaveButton;
  vtkKWPushButton*  ClearButton;
  vtkKWPushButton*  RefreshButton;
  vtkKWLabel*       ThresholdLabel;
  vtkKWOptionMenu*  ThresholdMenu;
  vtkKWLabel*       BufferLengthLabel;
  vtkKWOptionMenu*  BufferLengthMenu;
  vtkKWLabel*       EnableLabel;
  vtkKWCheckButton* EnableCheck;

  vtkKWText*        DisplayText;

  vtkKWWidget*     ButtonFrame;
  vtkKWPushButton* DismissButton;
    
  char*   Title;
  float   Threshold;

  vtkPVTimerInformation* TimerInformation;

private:
  vtkPVTimerLogDisplay(const vtkPVTimerLogDisplay&); // Not implemented
  void operator=(const vtkPVTimerLogDisplay&); // Not implemented
};

#endif
