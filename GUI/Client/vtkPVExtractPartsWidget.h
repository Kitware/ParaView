/*=========================================================================

  Program:   ParaView
  Module:    vtkPVExtractPartsWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVExtractPartsWidget - Widget for vtkSelectInputs filter.
// .SECTION Description
// This filter lets the user select a subset of parts fromthe input PVData.
// Since this widget will effect the outputs of the SelectInputsFilter,
// I will need to modify accept so the widgets accept methods get
// called before initialize data.
// I also have to add a widget option that fixes the widget value after
// accept is called.

#ifndef __vtkPVExtractPartsWidget_h
#define __vtkPVExtractPartsWidget_h

#include "vtkPVWidget.h"
class vtkKWPushButton;
class vtkKWWidget;
class vtkKWListBox;
class vtkCollection;

class VTK_EXPORT vtkPVExtractPartsWidget : public vtkPVWidget
{
public:
  static vtkPVExtractPartsWidget* New();
  vtkTypeRevisionMacro(vtkPVExtractPartsWidget, vtkPVWidget);
  void PrintSelf(ostream& os, vtkIndent indent);
    
  // Description:
  // Set up the UI for this source
  void Create(vtkKWApplication *app);

  // Description:
  // Save this source to a file.
  void SaveInBatchScript(ofstream *file);

  // Description:
  // Button callbacks.
  void AllOnCallback();
  void AllOffCallback();

  // Description:
  // Access metod necessary for scripting.
  void SetSelectState(int idx, int val);

  // Description:
  // This serves a dual purpose.  For tracing and for saving state.
  virtual void Trace(ofstream *file);

  // Description:
  // Called when the Accept button is pressed. It moves the widget state to
  // the SM property.
  virtual void Accept();

  // Description:
  // This method resets the widget values from the VTK filter.
  virtual void ResetInternal();
  virtual void Initialize();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();
 
  void PartSelectionCallback();

protected:
  vtkPVExtractPartsWidget();
  ~vtkPVExtractPartsWidget();

  vtkKWWidget* ButtonFrame;
  vtkKWPushButton* AllOnButton;
  vtkKWPushButton* AllOffButton;

  vtkKWListBox* PartSelectionList;
  // Labels get substituted for list box after accept is called.
  vtkCollection* PartLabelCollection;

  // Called to inactivate widget (after accept is called).
  void Inactivate();

  void CommonInit();

  vtkPVExtractPartsWidget(const vtkPVExtractPartsWidget&); // Not implemented
  void operator=(const vtkPVExtractPartsWidget&); // Not implemented
};

#endif
