/*=========================================================================

  Program:   ParaView
  Module:    vtkPVLineSourceWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVLineSourceWidget - a LineWidget which contains a separate line source
// .SECTION Description
// This widget contains a vtkPVLineWidget as well as a vtkLineSource. 
// This vtkLineSource (which is created on all processes) can be used as 
// input or source to filters (for example as streamline seed).

#ifndef __vtkPVLineSourceWidget_h
#define __vtkPVLineSourceWidget_h

#include "vtkPVSourceWidget.h"

class vtkPVLineWidget;

class VTK_EXPORT vtkPVLineSourceWidget : public vtkPVSourceWidget
{
public:
  static vtkPVLineSourceWidget* New();
  vtkTypeRevisionMacro(vtkPVLineSourceWidget, vtkPVSourceWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Creates common widgets.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // The underlying line widget.
  vtkGetObjectMacro(LineWidget, vtkPVLineWidget);

  // Description:
  // This method looks to the subwidgets to compute the modified flag.
  virtual int GetModifiedFlag();

  // Description:
  // This method is called when the source that contains this widget
  // is selected.
  virtual void Select();

  // Description:
  // This method is called when the source that contains this widget
  // is deselected.
  virtual void Deselect();

  // Description:
  // Saves the value of this widget into a VTK Tcl script.
  // This creates the line source (one for all parts).
  virtual void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // The methods get called when the Accept button is pressed. 
  // It sets the VTK objects value using this widgets value.
  virtual void Accept();
  //ETX

  // Description:
  // The methods get called when the Reset button is pressed. 
  // It sets this widgets value using the VTK objects value.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
protected:
  vtkPVLineSourceWidget();
  ~vtkPVLineSourceWidget();



  vtkPVLineWidget* LineWidget;

  vtkPVLineSourceWidget(const vtkPVLineSourceWidget&); // Not implemented
  void operator=(const vtkPVLineSourceWidget&); // Not implemented

};

#endif
