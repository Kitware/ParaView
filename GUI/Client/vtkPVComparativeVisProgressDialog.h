/*=========================================================================

  Program:   ParaView
  Module:    vtkPVComparativeVisProgressDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVComparativeVisProgressDialog -
// .SECTION Description
// .SECTION See Also
// vtkPVComparativeVisManagerGIO

#ifndef __vtkPVComparativeVisProgressDialog_h
#define __vtkPVComparativeVisProgressDialog_h

#include "vtkKWDialog.h"

class vtkKWProgressGauge;
class vtkKWFrame;
class vtkKWLabel;
class vtkKWPushButton;

class VTK_EXPORT vtkPVComparativeVisProgressDialog : public vtkKWDialog
{
public:
  static vtkPVComparativeVisProgressDialog* New();
  vtkTypeRevisionMacro(vtkPVComparativeVisProgressDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set and update the progress.
  void SetProgress(double prog);

  // Description:
  // This flag is checked by the gui during progress update.
  // If set to 1, comparative vis generation is aborted and
  // the flag is reset to 0 (by the gui).
  vtkSetMacro(AbortFlag, int);
  vtkGetMacro(AbortFlag, int);

protected:
  vtkPVComparativeVisProgressDialog();
  ~vtkPVComparativeVisProgressDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();

  vtkKWFrame* ProgressFrame;
  vtkKWLabel* ProgressLabel;
  vtkKWProgressGauge* ProgressBar;
  vtkKWLabel* Message;
  vtkKWPushButton* CancelButton;

  int AbortFlag;

private:
  vtkPVComparativeVisProgressDialog(const vtkPVComparativeVisProgressDialog&); // Not implemented
  void operator=(const vtkPVComparativeVisProgressDialog&); // Not implemented
};


#endif


