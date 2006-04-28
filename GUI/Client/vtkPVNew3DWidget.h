/*=========================================================================

  Program:   ParaView
  Module:    vtkPVNew3DWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVNew3DWidget - Test widget to demonstrate the new 3D widgets
// .SECTION Description
// vtkPVNew3DWidget demonstrates the server manager support for the new
// 3D widgets. It is not a general purpose widget and should be removed
// before 2.4.6.

#ifndef __vtkPVNew3DWidget_h
#define __vtkPVNew3DWidget_h

#include "vtkPVObjectWidget.h"

class vtkKWLabel;
class vtkKWEntry;
class vtkSMNew3DWidgetProxy;
class vtkPVNew3DWidgetObserver;

class VTK_EXPORT vtkPVNew3DWidget : public vtkPVObjectWidget
{
public:
  static vtkPVNew3DWidget* New();
  vtkTypeRevisionMacro(vtkPVNew3DWidget, vtkPVObjectWidget);
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

  // Description:
  // Internal.
  void EntryModifiedCallback(const char* key);

protected:
  vtkPVNew3DWidget();
  ~vtkPVNew3DWidget();

  // Description:
  // Create the widget. 
  virtual void CreateWidget();
  
  void WidgetModified();

//BTX
  friend class vtkPVNew3DWidgetObserver;
//ETX

  vtkKWLabel* LabelWidget;
  vtkKWEntry* EntryWidget;

  vtkSMNew3DWidgetProxy* WidgetProxy;
  vtkPVNew3DWidgetObserver* Observer;
private:  
  vtkPVNew3DWidget(const vtkPVNew3DWidget&); // Not implemented
  void operator=(const vtkPVNew3DWidget&); // Not implemented
};

#endif
