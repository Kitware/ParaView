/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPVApplicationSettingsInterface.h
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
// .NAME vtkPVApplicationSettingsInterface - a user interface panel.
// .SECTION Description
// A concrete implementation of a user interface panel. It extends the
// user interface defined in vtkKWApplicationSettingsInterface.
// See vtkKWUserInterfacePanel for a more detailed description.
// .SECTION See Also
// vtkKWApplicationSettingsInterface vtkKWUserInterfacePanel vtkKWUserInterfaceManager vtkKWUserInterfaceNotebookManager

#ifndef __vtkPVApplicationSettingsInterface_h
#define __vtkPVApplicationSettingsInterface_h

#include "vtkKWApplicationSettingsInterface.h"

//------------------------------------------------------------------------------

#define VTK_PV_ASI_TOOLBAR_FLAT_FRAME_REG_KEY       "ToolbarFlatFrame"
#define VTK_PV_ASI_TOOLBAR_FLAT_BUTTONS_REG_KEY     "ToolbarFlatButtons"

//------------------------------------------------------------------------------

class vtkKWCheckButton;
class vtkKWLabeledFrame;
class vtkPVWindow;

class VTK_EXPORT vtkPVApplicationSettingsInterface : public vtkKWApplicationSettingsInterface
{
public:
  static vtkPVApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkPVApplicationSettingsInterface,vtkKWApplicationSettingsInterface);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the window (do not ref count it since the window will ref count
  // this widget).
  vtkGetObjectMacro(Window, vtkPVWindow);
  virtual void SetWindow(vtkPVWindow*);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Set ShowSourcesDescription UI On/Off programmatically. 
  // Save the setting to registry.
  // Do not call vtkPVWindow::SetShowSourcesLongHelp(), since it
  // is likely to call us to update the UI programmatically (note that the 
  // callback will call it on user interaction though).
  void SetShowSourcesDescription(int);

  // Description:
  // Set ShowSourcesName UI On/Off programmatically. 
  // Save the setting to registry.
  // Do not call vtkPVRenderView::SetSourcesBrowserAlwaysShowName(), since it
  // is likely to call us to update the UI programmatically (note that the 
  // callback will call it on user interaction though).
  void SetShowSourcesName(int);

  // Description:
  // Set FlatFrame UI On/Off programmatically. 
  // Save the setting to registry.
  // Call vtkPVWindow::UpdateToolbarAspect() to update the toolbar aspect.
  void SetFlatFrame(int);

  // Description:
  // Set FlatFrame UI On/Off programmatically. 
  // Save the setting to registry.
  // Call vtkPVWindow::UpdateToolbarAspect() to update the toolbar aspect.
  void SetFlatButtons(int);

  // Description:
  // Callback used when interaction has been performed.
  void ShowSourcesDescriptionCallback();
  void ShowSourcesNameCallback();
  void FlatFrameCallback();
  void FlatButtonsCallback();

  // Description:
  // Access to some sub-widgets. Note that they might be NULL until the Create()
  // function is called.
  vtkGetObjectMacro(ToolbarSettingsFrame, vtkKWLabeledFrame);

protected:
  vtkPVApplicationSettingsInterface();
  ~vtkPVApplicationSettingsInterface();

  vtkPVWindow       *Window;

  vtkKWCheckButton *ShowSourcesDescriptionCheckButton;
  vtkKWCheckButton *ShowSourcesNameCheckButton;

  vtkKWLabeledFrame *ToolbarSettingsFrame;
  vtkKWCheckButton  *FlatFrameCheckButton;
  vtkKWCheckButton  *FlatButtonsCheckButton;

  // Update the enable state. This should propagate similar calls to the
  // internal widgets.
  virtual void UpdateEnableState();

private:
  vtkPVApplicationSettingsInterface(const vtkPVApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkPVApplicationSettingsInterface&); // Not Implemented
};

#endif
