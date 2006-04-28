/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNewImplicitPlane.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVNewImplicitPlane - Test widget to demonstrate the new 3D widgets
// .SECTION Description
// vtkPVNewImplicitPlane demonstrates the server manager support for the new
// 3D widgets. It is not a general purpose widget and should be removed
// before 2.4.6.

#ifndef __vtkPVNewImplicitPlane_h
#define __vtkPVNewImplicitPlane_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWEntry;
class vtkSMNew3DWidgetProxy;
class vtkPVNewImplicitPlaneObserver;
class vtkSMProxy;

class VTK_EXPORT vtkPVNewImplicitPlane : public vtkPVObjectWidget
{
public:
  static vtkPVNewImplicitPlane* New();
  vtkTypeRevisionMacro(vtkPVNewImplicitPlane, vtkPVObjectWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This method is called when the source that contains this widget
  // is selected. 
  virtual void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected. 
  virtual void Deselect();

  //BTX
  // Description:
  // Move widget state to VTK object or back.
  virtual void Accept();
  virtual void ResetInternal();

  // Description:
  // Initialize the newly created widget.
  virtual void Initialize();
  //ETX

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Save this widget to a file.
  virtual void SaveInBatchScript(ofstream *) {};

protected:
  vtkPVNewImplicitPlane();
  ~vtkPVNewImplicitPlane();

  // Description:
  // Create the widget. 
  virtual void CreateWidget();
  
//BTX
  friend class vtkPVNewImplicitPlaneObserver;
//ETX

  vtkSMNew3DWidgetProxy* WidgetProxy;
  vtkSMProxy* ImplicitPlaneProxy;
  vtkPVNewImplicitPlaneObserver* Observer;

private:  
  vtkPVNewImplicitPlane(const vtkPVNewImplicitPlane&); // Not implemented
  void operator=(const vtkPVNewImplicitPlane&); // Not implemented
};

#endif
