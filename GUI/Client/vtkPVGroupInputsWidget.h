/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGroupInputsWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGroupInputsWidget - Widget for vtkGroup filter.
// .SECTION Description
// This widget lets the user select multiple inputs for the vtkGroup filter.
// After accept is called, the widget is disabled.  This is necessary
// because we cannot allow the number of outputs or output types
// to change after the outputs have been created.

#ifndef __vtkPVGroupInputsWidget_h
#define __vtkPVGroupInputsWidget_h

#include "vtkPVWidget.h"

class vtkKWPushButton;
class vtkKWWidget;
class vtkKWListBox;
class vtkCollection;
class vtkPVSourceCollection;
class vtkPVSourceVectorInternals;

class VTK_EXPORT vtkPVGroupInputsWidget : public vtkPVWidget
{
public:
  static vtkPVGroupInputsWidget* New();
  vtkTypeRevisionMacro(vtkPVGroupInputsWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void Create(vtkKWApplication *app);

  // Description:
  // Save this source to a file.
  void SaveInBatchScript(ofstream *file);

  //BTX
  // Description:
  // Called when the Accept button is pressed.  It moves the widget values to the 
  // VTK filter.
  virtual void Accept();
  //ETX

  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Access metod necessary for scripting.
  void SetSelectState(vtkPVSource* input, int val);

  // Description:
  // No buttons yet, just used for tracing.
  void AllOnCallback();
  void AllOffCallback();


protected:
  vtkPVGroupInputsWidget();
  ~vtkPVGroupInputsWidget();

  vtkPVSourceVectorInternals *Internal;

  vtkKWListBox* PartSelectionList;
  // Labels get substituted for list box after accept is called.
  vtkCollection* PartLabelCollection;


  // Called to inactivate widget (after accept is called).
  void Inactivate();

  
  vtkPVGroupInputsWidget(const vtkPVGroupInputsWidget&); // Not implemented
  void operator=(const vtkPVGroupInputsWidget&); // Not implemented
};

#endif
