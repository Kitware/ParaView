/*=========================================================================

  Program:   ParaView
  Module:    vtkPVApplicationSettingsInterface.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

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

//----------------------------------------------------------------------------

#define VTK_PV_ASI_SHOW_SOURCES_DESCRIPTION_REG_KEY "ShowSourcesLongHelp"
#define VTK_PV_ASI_SHOW_SOURCES_NAME_REG_KEY    "SourcesBrowserAlwaysShowName"
#define VTK_PV_ASI_SHOW_TRACE_FILES_REG_KEY "ShowTraceFiles"

class vtkKWCheckButton;
class vtkKWFrameLabeled;

class VTK_EXPORT vtkPVApplicationSettingsInterface : public vtkKWApplicationSettingsInterface
{
public:
  static vtkPVApplicationSettingsInterface* New();
  vtkTypeRevisionMacro(vtkPVApplicationSettingsInterface,vtkKWApplicationSettingsInterface);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the interface objects.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Refresh the interface given the current value of the Window and its
  // views/composites/widgets.
  virtual void Update();

  // Description:
  // Callback used when interaction has been performed.
  virtual void AutoAcceptCallback();
  virtual void ShowSourcesDescriptionCallback();
  virtual void ShowSourcesNameCallback();
  virtual void ShowTraceFilesCallback();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // Auto accept can be set by the accept button pull down method.
  void SetAutoAccept(int val);

protected:
  vtkPVApplicationSettingsInterface();
  ~vtkPVApplicationSettingsInterface();

  vtkKWCheckButton *ShowSourcesDescriptionCheckButton;
  vtkKWCheckButton *ShowSourcesNameCheckButton;
  vtkKWCheckButton *ShowTraceFilesCheckButton;
  vtkKWCheckButton *AutoAcceptCheckButton;

  int AutoAccept;

private:
  vtkPVApplicationSettingsInterface(const vtkPVApplicationSettingsInterface&); // Not implemented
  void operator=(const vtkPVApplicationSettingsInterface&); // Not Implemented
};

#endif
