/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWApplicationSettingsInterface.h
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
// .NAME vtkKWApplicationSettingsInterface - a user interface panel.
// .SECTION Description
// A concrete implementation of a user interface panel.
// See vtkKWUserInterfacePanel for a more detailed description.
// .SECTION See Also
// vtkKWUserInterfacePanel vtkKWUserInterfaceManager vtkKWUserInterfaceNotebookManager

#ifndef __vtkKWApplicationSettingsInterface_h
#define __vtkKWApplicationSettingsInterface_h

#include "vtkKWUserInterfacePanel.h"

//------------------------------------------------------------------------------

#define VTK_KW_ASI_PREFERENCES_LABEL            "Preferences"

#define VTK_KW_ASI_SAVE_WINDOW_GEOMETRY_REG_KEY "SaveWindowGeometry"
#define VTK_KW_ASI_SHOW_SPLASH_SCREEN_REG_KEY   "ShowSplashScreen"
#define VTK_KW_ASI_SHOW_TOOLTIPS_REG_KEY        "ShowBalloonHelp"

//------------------------------------------------------------------------------

class vtkKWLabeledFrame;
class vtkKWCheckButton;

class VTK_EXPORT vtkKWApplicationSettingsInterface : public vtkKWUserInterfacePanel
{
public:
  static vtkKWApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkKWApplicationSettingsInterface,vtkKWUserInterfacePanel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Enable/Disable this panel. This should propagate SetEnabled() calls to the
  // internal widgets.
  virtual void SetEnabled(int e);

  // Description:
  // Set ConfirmExit UI On/Off programmatically. 
  // Save the setting to registry.
  void SetConfirmExit(int);

  // Description:
  // Set SaveGeometry UI On/Off programmatically. 
  // Save the setting to registry.
  void SetSaveGeometry(int);

  // Description:
  // Set ShowSplashScreen UI On/Off programmatically. 
  // Save the setting to registry.
  // Call the vtkKWapplication::SetShowSplashScreen() method accordingly.
  void SetShowSplashScreen(int);

  // Description:
  // Set ShowBalloonHelp (tooltips) UI On/Off programmatically. 
  // Save the setting to registry.
  // Call the vtkKWapplication::SetShowBalloonHelp() method accordingly.
  void SetShowBalloonHelp(int);

  // Description:
  // Callback used when interaction has been performed.
  void ConfirmExitCallback();
  void SaveGeometryCallback();
  void ShowSplashScreenCallback();
  void ShowBalloonHelpCallback();

  // Description:
  // Access to some sub-widgets. Note that they might be NULL until the Create()
  // function is called.
  vtkGetObjectMacro(InterfaceSettingsFrame, vtkKWLabeledFrame);

  // Description:
  // Refresh the interface given the ivars.
  virtual void Update() {};

protected:
  vtkKWApplicationSettingsInterface();
  ~vtkKWApplicationSettingsInterface();

  vtkKWLabeledFrame *InterfaceSettingsFrame;

  vtkKWCheckButton  *ConfirmExitCheckButton;
  vtkKWCheckButton  *SaveGeometryCheckButton;
  vtkKWCheckButton  *ShowSplashScreenCheckButton;
  vtkKWCheckButton  *ShowBalloonHelpCheckButton;

private:
  vtkKWApplicationSettingsInterface(const vtkKWApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkKWApplicationSettingsInterface&); // Not Implemented
};

#endif
