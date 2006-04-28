/*=========================================================================

  Module:    vtkPVComparativeVisPropertyWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisPropertyWidget - widget for setting up one parameter of comparative vis
// .SECTION Description
// This class describes a widget designed for setting up one property
// (parameter) of a comparative visualization. It has a track editor
// for choosing the property and an entry to enter the number of
// parameter values.
// .SECTION See Also
// vtkSMComparativeVisProxy

#ifndef __vtkPVComparativeVisPropertyWidget_h
#define __vtkPVComparativeVisPropertyWidget_h

#include "vtkKWCompositeWidget.h"

class vtkKWEntryWithLabel;
class vtkPVActiveTrackSelector;
class vtkPVAnimationCue;
class vtkPVSimpleAnimationCue;
class vtkPVSource;
class vtkPVTrackEditor;
class vtkSMProxy;
class vtkSMComparativeVisProxy;

class VTK_EXPORT vtkPVComparativeVisPropertyWidget : public vtkKWCompositeWidget
{
public:
  static vtkPVComparativeVisPropertyWidget* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisPropertyWidget,vtkKWCompositeWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Copy the values of the widget to the given vis.
  void CopyToVisualization(vtkSMComparativeVisProxy* cv);

  // Description:
  // Copy the values of the given vis to the widget.
  void CopyFromVisualization(
    unsigned int propIdx, vtkSMComparativeVisProxy* proxy);

  // Description:
  // Show the cue editor for the currently selected property in
  // the given frame.
  void ShowCueEditor();

  // Description:
  // Set/Get the track editor. Should be set before Create().
  void SetTrackEditor(vtkPVTrackEditor* ed);
  vtkGetObjectMacro(TrackEditor, vtkPVTrackEditor);

protected:
  vtkPVComparativeVisPropertyWidget();
  ~vtkPVComparativeVisPropertyWidget();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkPVActiveTrackSelector* TrackSelector;
  vtkPVSimpleAnimationCue* CueEditor;
  vtkPVAnimationCue* LastCue;
  vtkKWEntryWithLabel* NumberOfFramesEntry;
  vtkPVTrackEditor* TrackEditor;

  void RemovePVSource(vtkPVSource* source);

//BTX
  friend class vtkPVCVSourceDeletedCommand;
//ETX

private:
  vtkPVComparativeVisPropertyWidget(const vtkPVComparativeVisPropertyWidget&); // Not implemented
  void operator=(const vtkPVComparativeVisPropertyWidget&); // Not implemented
};

#endif
