/*=========================================================================

  Program:   ParaView
  Module:    vtkPVErrorLogDisplay.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVErrorLogDisplay - Shows a text version of the timer log entries.
// .SECTION Description
// A widget to display timing information in the timer log.

#ifndef __vtkPVErrorLogDisplay_h
#define __vtkPVErrorLogDisplay_h

#include "vtkPVTimerLogDisplay.h"

class vtkKWApplication;
class vtkKWPushButton;
class vtkKWEntry;
class vtkKWLabel;
class vtkKWText;
class vtkKWWindow;
class vtkKWOptionMenu;
class vtkKWCheckButton;

//BTX
template<class t> class vtkVector;
//ETX

class VTK_EXPORT vtkPVErrorLogDisplay : public vtkPVTimerLogDisplay
{
public:
  static vtkPVErrorLogDisplay* New();
  vtkTypeRevisionMacro(vtkPVErrorLogDisplay, vtkPVTimerLogDisplay);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Add an error to the list.
  virtual void AppendError(const char*);
  
  // Description:
  // Clear all entries from the buffer.
  virtual void Clear();

  // Description:
  // Saves the current log to a file.
  virtual void Save(const char* fileName);

protected:
  vtkPVErrorLogDisplay();
  ~vtkPVErrorLogDisplay();

  virtual void Update();

  //BTX
  vtkVector<const char*>* ErrorMessages;
  //ETX
  
private:
  vtkPVErrorLogDisplay(const vtkPVErrorLogDisplay&); // Not implemented
  void operator=(const vtkPVErrorLogDisplay&); // Not implemented
};

#endif
