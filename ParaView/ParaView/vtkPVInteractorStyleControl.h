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
// This widget defines a user interface for controling interactor
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
class vtkKWLabeledFrame;
class vtkKWOptionMenu;
class vtkPVCameraManipulator;
class vtkPVInteractorStyleControlCmd;
class vtkPVWidget;

//BTX
template<class KeyType,class DataType> class vtkArrayMap;
template<class KeyType,class DataType> class vtkArrayMapIterator;
template<class DataType> class vtkVector;
//ETX

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
  vtkGetObjectMacro(LabeledFrame, vtkKWLabeledFrame);

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
  int SetManipulator(int mouse, int key, const char*);
  vtkPVCameraManipulator* GetManipulator(int pos);
  vtkPVCameraManipulator* GetManipulator(int mouse, int key);
  vtkPVCameraManipulator* GetManipulator(const char* name);

  // Description:
  // Set the current manipulator to the specified one for the
  // mouse button and keypress combination.
  void SetCurrentManipulator(int pos, const char*);
  void SetCurrentManipulator(int mouse, int key, const char*);

  // Description:
  // In order for manipulators to work, you have to set them
  // on the window. This method sets the window.
  void SetManipulatorCollection(vtkCollection*);
  vtkGetObjectMacro(ManipulatorCollection, vtkCollection);

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

  // Description
  // This is hack to convert the current manipulator to Tcl variable.
  vtkGetObjectMacro(CurrentManipulator, vtkPVCameraManipulator);

  // Description:
  // This method is called when one of the manipulator is modified.
  void ExecuteEvent(vtkObject* wdg, unsigned long event, void* calldata);

protected:
  vtkPVInteractorStyleControl();
  ~vtkPVInteractorStyleControl();

  vtkKWLabeledFrame *LabeledFrame;
  vtkKWLabel *Labels[6];
  vtkKWOptionMenu *Menus[9];
  vtkKWFrame *ArgumentsFrame;
  vtkKWFrame *MenusFrame;

  vtkPVInteractorStyleControlCmd *Observer;

  int InEvent;

  vtkCollection *ManipulatorCollection;
  char* DefaultManipulator;
  char* RegisteryName;

//BTX
  typedef vtkVector<const char*> ArrayStrings;
  typedef vtkArrayMap<const char*,vtkPVCameraManipulator*> ManipulatorMap;
  typedef vtkArrayMapIterator<const char*,vtkPVCameraManipulator*> 
    ManipulatorMapIterator;
  typedef vtkArrayMap<const char*,vtkPVWidget*> WidgetsMap;
  typedef vtkArrayMap<const char*,ArrayStrings*> MapStringToArrayStrings;

  ManipulatorMap*          Manipulators;
  WidgetsMap*              Widgets;
  vtkCollection*           WidgetProperties;
  MapStringToArrayStrings* Arguments;
//ETX

  // This is hack to get tcl name;
  vtkPVCameraManipulator *CurrentManipulator;

private:
  vtkPVInteractorStyleControl(const vtkPVInteractorStyleControl&); // Not implemented
  void operator=(const vtkPVInteractorStyleControl&); // Not implemented
};


#endif


