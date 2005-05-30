/*=========================================================================

  Module:    vtkPVComparativeVisWidget.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisWidget -
// .SECTION Description

#ifndef __vtkPVComparativeVisWidget_h
#define __vtkPVComparativeVisWidget_h

#include "vtkKWWidget.h"

class vtkPVActiveTrackSelector;
class vtkPVAnimationCue;
class vtkPVComparativeVis;
class vtkPVSimpleAnimationCue;
class vtkPVTrackEditor;
class vtkSMProxy;

class VTK_EXPORT vtkPVComparativeVisWidget : public vtkKWWidget
{
public:
  static vtkPVComparativeVisWidget* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisWidget,vtkKWWidget);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app, const char* args);

  // Description:
  // Copy the values of the widget to the given vis.
  void CopyToVisualization(vtkPVComparativeVis* cv);

  // Description:
  // Copy the values of the given vis to the widget.
  void CopyFromVisualization(vtkPVAnimationCue* cue);

  // Description:
  // Show the cue editor for the currently selected property in
  // the given frame. This will create a new vtkPVSimpleAnimationCue
  // if necessary.
  void ShowCueEditor(vtkPVTrackEditor* ed);

protected:
  vtkPVComparativeVisWidget();
  ~vtkPVComparativeVisWidget();

  vtkPVActiveTrackSelector* TrackSelector;
  vtkPVSimpleAnimationCue* LastCueEditor;
  vtkPVAnimationCue* LastCue;

private:
  vtkPVComparativeVisWidget(const vtkPVComparativeVisWidget&); // Not implemented
  void operator=(const vtkPVComparativeVisWidget&); // Not implemented
};

#endif
