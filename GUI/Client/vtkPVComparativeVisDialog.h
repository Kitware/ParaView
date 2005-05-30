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
// .NAME vtkPVComparativeVisDialog -
// .SECTION Description

#ifndef __vtkPVComparativeVisDialog_h
#define __vtkPVComparativeVisDialog_h

#include "vtkKWMessageDialog.h"

class vtkPVComparativeVis;
class vtkPVComparativeVisWidget;
class vtkPVTrackEditor;
//BTX
struct vtkPVComparativeVisDialogInternals;
//ETX

class VTK_EXPORT vtkPVComparativeVisDialog : public vtkKWMessageDialog
{
public:
  static vtkPVComparativeVisDialog* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisDialog,vtkKWMessageDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // Copy the values of the dialog to the given vis. Call after and if Invoke()
  // returns true.
  void CopyToVisualization(vtkPVComparativeVis* cv);

  // Description:
  // Create n property widgets where n is the number of properties in cv.
  void InitializeFromVisualization(vtkPVComparativeVis* cv);

  // Description:
  // Copy the values of the given vis to the dialog. Call after Create().
  void CopyFromVisualization(vtkPVComparativeVis* cv);

  void InitializeToDefault();

protected:
  vtkPVComparativeVisDialog();
  ~vtkPVComparativeVisDialog();

  vtkPVComparativeVisDialogInternals* Internal;

  void CueSelected(vtkPVComparativeVisWidget* wid);

//BTX
  friend class vtkCueSelectionCommand;
//ETX

  vtkPVTrackEditor* TrackEditor;

private:
  vtkPVComparativeVisDialog(const vtkPVComparativeVisDialog&); // Not implemented
  void operator=(const vtkPVComparativeVisDialog&); // Not implemented
};


#endif


