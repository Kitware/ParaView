/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisDialog - dialog for editing/create a comparative visualization
// .SECTION Description
// vtkPVComparativeVisDialog provides user interface for creating/editing
// comparative visualizations.
// .SECTION See Also
// vtkPVComparativeVis

#ifndef __vtkPVComparativeVisDialog_h
#define __vtkPVComparativeVisDialog_h

#include "vtkKWDialog.h"

class vtkKWEntryLabeled;
class vtkKWFrame;
class vtkKWFrameLabeled;
class vtkKWPushButton;
class vtkPVComparativeVis;
class vtkPVComparativeVisPropertyWidget;
class vtkPVTrackEditor;
//BTX
struct vtkPVComparativeVisDialogInternals;
//ETX

class VTK_EXPORT vtkPVComparativeVisDialog : public vtkKWDialog
{
public:
  static vtkPVComparativeVisDialog* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create the widget.
  virtual void Create(vtkKWApplication *app);

  // Description:
  // Copy the values of the dialog to the given vis. Call after and if Invoke()
  // returns true.
  void CopyToVisualization(vtkPVComparativeVis* cv);

  // Description:
  // Copy the values of the given vis to the dialog. Call after Create().
  void CopyFromVisualization(vtkPVComparativeVis* cv);

  // Description:
  // The default case is two (unselected) properties with 5 values.
  void InitializeToDefault();

  // Description:
  // Callback bound to the radio buttons.
  void CueSelected(unsigned int i);

protected:
  vtkPVComparativeVisDialog();
  ~vtkPVComparativeVisDialog();
  
  // Called when user selects a property
  void CueSelected(vtkPVComparativeVisPropertyWidget* wid);

  // Create a new property widget. 
  void NewPropertyWidget();

//BTX
  friend class vtkCueSelectionCommand;
//ETX

  // To edit the keyframes
  vtkPVTrackEditor* TrackEditor;
  // The name of the visualization
  vtkKWEntryLabeled* NameEntry;
  // The property list
  vtkKWFrameLabeled* VisualizationListFrame;

  vtkKWFrame* MainFrame;
  vtkKWPushButton* CloseButton;

  // Used in assigning unique default names to visualization
  static int NumberOfVisualizationsCreated;

  // Control the (fixed) dimensions of the dialog
  static const int DialogWidth;
  static const int DialogHeight;

private:
  vtkPVComparativeVisDialog(const vtkPVComparativeVisDialog&); // Not implemented
  void operator=(const vtkPVComparativeVisDialog&); // Not implemented

  // PIMPL
  vtkPVComparativeVisDialogInternals* Internal;
};


#endif


