/*=========================================================================

  Program:   ParaView
  Module:    vtkPVInteractorStyleControl.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVInteractorStyleControl - a control widget for manipulators
// .SECTION Description
// This widget defines a user interface for controlling interactor
// style. It defines nine menus for different button and keyboard
// combinations and bind a manipulator for each one of them. It also
// provides a simple user interface for some manipulators.
// 


#ifndef __vtkPVInteractorStyleControl_h
#define __vtkPVInteractorStyleControl_h

#include "vtkKWWidget.h"

class vtkCollection;
class vtkKWApplication;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWFrameLabeled;
class vtkKWOptionMenu;
class vtkPVCameraManipulator;
class vtkPVInteractorStyleControlCmd;
class vtkPVWidget;

class vtkPVInteractorStyleControlInternal;

class VTK_EXPORT vtkPVInteractorStyleControl : public vtkKWWidget
{  
public:
  static vtkPVInteractorStyleControl* New();
  vtkTypeRevisionMacro(vtkPVInteractorStyleControl,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char*);

  // Description:
  // Get the vtkKWWidget for the internal frame.
  vtkGetObjectMacro(LabeledFrame, vtkKWFrameLabeled);

  // Description:
  // Add manipulator to the list of manipulators.
  void AddManipulator(const char*, vtkPVCameraManipulator*);

  // Description:
  // Update menus after adding manipulators.
  void UpdateMenus();

  // Description:
  // Set label of the control widget.
  void SetLabel(const char*);

  // Description:
  // Set the specific manipulator for a mouse button and key
  // combination.
  int SetManipulator(int pos, const char*);
  vtkPVCameraManipulator* GetManipulator(int pos);
  vtkPVCameraManipulator* GetManipulator(const char* name);

  // Description:
  // Set the current manipulator to the specified one for the
  // mouse button and keypress combination.
  void SetCurrentManipulator(int pos, const char*);
  void SetCurrentManipulator(int mouse, int key, const char*);

  //BTX
  // Description:
  // In order for manipulators to work, you have to set them
  // on the window. This method sets the window.
  void SetManipulatorCollection(vtkCollection*);
  vtkGetObjectMacro(ManipulatorCollection, vtkCollection);
  //ETX

  // Description:
  // Set or get the default manipulator. The default manipulator is
  // the one that is present in menus (after UpdateMenus) which do not
  // have any manipulator set.
  vtkSetStringMacro(DefaultManipulator);
  vtkGetStringMacro(DefaultManipulator);

  // Description:
  // Read and store information to the registery.
  void ReadRegistery();
  void StoreRegistery();

  // Description:
  // Type or name of manipulator is used for storing in the registery.
  vtkSetStringMacro(RegisteryName);
  vtkGetStringMacro(RegisteryName);

  // Description:
  // Add argument that can be modified for specific manipulator.
  void AddArgument(const char* name, const char* manipulator,
                   vtkPVWidget* widget);

  // Description:
  // Callback for widget to call when user modifies UI.
  void ChangeArgument(const char* name, const char* widget);
  void ResetWidget(vtkPVCameraManipulator*, const char* name);

  // Description:
  // Get a widget by name
  vtkPVWidget* GetWidget(const char* name);
  
  // Description
  // This is hack to convert the current manipulator to Tcl variable.
  vtkGetObjectMacro(CurrentManipulator, vtkPVCameraManipulator);

  // Description:
  // This method is called when one of the manipulator is modified.
  void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

  // Description:
  // Export the state of the interactor style to a file.
  virtual void SaveState(ofstream *file);
 
  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVInteractorStyleControl();
  ~vtkPVInteractorStyleControl();

  vtkKWFrameLabeled *LabeledFrame;
  vtkKWFrame        *OuterFrame;
  vtkKWLabel *Labels[6];
  vtkKWOptionMenu *Menus[9];
  vtkKWFrame *ArgumentsFrame;

  vtkPVInteractorStyleControlCmd *Observer;

  int InEvent;

  vtkCollection *ManipulatorCollection;
  char* DefaultManipulator;
  char* RegisteryName;

  // This is hack to get tcl name;
  vtkPVCameraManipulator *CurrentManipulator;

  vtkPVInteractorStyleControlInternal* Internals;

private:
  vtkPVInteractorStyleControl(const vtkPVInteractorStyleControl&); // Not implemented
  void operator=(const vtkPVInteractorStyleControl&); // Not implemented
};


#endif


