/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
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

//----------------------------------------------------------------------------

#define VTK_KW_SAVE_WINDOW_GEOMETRY_REG_KEY   "SaveWindowGeometry"
#define VTK_KW_SHOW_SPLASH_SCREEN_REG_KEY     "ShowSplashScreen"
#define VTK_KW_SHOW_TOOLTIPS_REG_KEY          "ShowBalloonHelp"

#define VTK_KW_TOOLBAR_FLAT_FRAME_REG_KEY     "ToolbarFlatFrame"
#define VTK_KW_TOOLBAR_FLAT_BUTTONS_REG_KEY   "ToolbarFlatButtons"

//----------------------------------------------------------------------------

class vtkKWLabeledFrame;
class vtkKWCheckButton;
class vtkKWWindow;

class VTK_EXPORT vtkKWApplicationSettingsInterface : public vtkKWUserInterfacePanel
{
public:
  static vtkKWApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkKWApplicationSettingsInterface,vtkKWUserInterfacePanel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window (do not ref count it since the window will ref count
  // this widget).
  vtkGetObjectMacro(Window, vtkKWWindow);
  virtual void SetWindow(vtkKWWindow*);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Callback used when interaction has been performed.
  virtual void ConfirmExitCallback();
  virtual void SaveGeometryCallback();
  virtual void ShowSplashScreenCallback();
  virtual void ShowBalloonHelpCallback();
  virtual void FlatFrameCallback();
  virtual void FlatButtonsCallback();

protected:
  vtkKWApplicationSettingsInterface();
  ~vtkKWApplicationSettingsInterface();

  vtkKWWindow       *Window;

  // Interface settings

  vtkKWLabeledFrame *InterfaceSettingsFrame;

  vtkKWCheckButton  *ConfirmExitCheckButton;
  vtkKWCheckButton  *SaveGeometryCheckButton;
  vtkKWCheckButton  *ShowSplashScreenCheckButton;
  vtkKWCheckButton  *ShowBalloonHelpCheckButton;

  // Toolbar settings

  vtkKWLabeledFrame *ToolbarSettingsFrame;
  vtkKWCheckButton  *FlatFrameCheckButton;
  vtkKWCheckButton  *FlatButtonsCheckButton;

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkKWApplicationSettingsInterface(const vtkKWApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkKWApplicationSettingsInterface&); // Not Implemented
};

#endif


