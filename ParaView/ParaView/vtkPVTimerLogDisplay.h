/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVTimerLogDisplay.h
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
// .NAME vtkPVTimerLogDisplay - Shows a text version of the timer log entries.
// .SECTION Description
// A widget to display timing information in the timer log.

#ifndef __vtkPVTimerLogDisplay_h
#define __vtkPVTimerLogDisplay_h

#include "vtkKWWidget.h"
class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWText;
class vtkKWWindow;
class vtkKWOptionMenu;
class vtkKWCheckButton;

class VTK_EXPORT vtkPVTimerLogDisplay : public vtkKWWidget
{
public:
  static vtkPVTimerLogDisplay* New();
  vtkTypeMacro(vtkPVTimerLogDisplay, vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  void Create(vtkKWApplication *app);
  
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

protected:
  vtkPVTimerLogDisplay();
  ~vtkPVTimerLogDisplay();

  virtual void Update();

  // Description:
  // Open log for writing.
  void EnableWrite();

  // Description:
  // Close log for writing
  void DisableWrite();

  void Append(const char*);
  
  vtkKWWindow*      MasterWindow;

  vtkKWWidget*      ControlFrame;
  vtkKWPushButton*  SaveButton;
  vtkKWPushButton*  ClearButton;
  vtkKWLabel*       ThresholdLabel;
  vtkKWOptionMenu*  ThresholdMenu;
  vtkKWLabel*       BufferLengthLabel;
  vtkKWOptionMenu*  BufferLengthMenu;
  vtkKWLabel*       EnableLabel;
  vtkKWCheckButton* EnableCheck;

  vtkKWWidget*      DisplayFrame;
  vtkKWText*        DisplayText;
  vtkKWWidget*      DisplayScrollBar;

  vtkKWWidget*     ButtonFrame;
  vtkKWPushButton* DismissButton;
    
  char*   Title;
  float   Threshold;
  int     Writable;

private:
  vtkPVTimerLogDisplay(const vtkPVTimerLogDisplay&); // Not implemented
  void operator=(const vtkPVTimerLogDisplay&); // Not implemented
};

#endif
