/*=========================================================================

  Program:   ParaView
  Module:    vtkPVGhostLevelDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVGhostLevelDialog - asks the user which ghostlevel he wants
// .SECTION Description
// Asks the user which ghostlevel he wants.
// .SECTION See Also
// vtkPVApplication

#ifndef __vtkPVGhostLevelDialog_h
#define __vtkPVGhostLevelDialog_h

#include "vtkKWDialog.h"

class vtkKWFrame;
class vtkKWLabel;
class vtkKWPushButton;
class vtkKWWidget;

class VTK_EXPORT vtkPVGhostLevelDialog : public vtkKWDialog
{
public:
  static vtkPVGhostLevelDialog* New();
  vtkTypeRevisionMacro(vtkPVGhostLevelDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Create a Tk widget
  virtual void Create(vtkKWApplication *app, const char *args);

  // Description:
  // GhostLevel selected by the user. SetGhostLevel is for
  // internal use only.
  void SetGhostLevel(int level);
  vtkGetMacro(GhostLevel, int);

  // Description:
  // Invoke the dialog and display it in a modal manner. 
  // This method returns a zero if the dilaog was killed or 
  // canceled, nonzero otherwise. After Invoke(), the
  // ghostlevel chosen by the user can by obtained with
  // GetGhostLevel
  virtual int Invoke();

protected:
  vtkPVGhostLevelDialog();
  ~vtkPVGhostLevelDialog();

  int GhostLevel;

  vtkKWFrame* Separator;
  vtkKWLabel* Label;
  vtkKWFrame* ButtonFrame;

  vtkKWFrame *SelFrame1;
  vtkKWFrame *SelFrame2;
  vtkKWFrame *SelFrame3;

  vtkKWPushButton* SelButton1;
  vtkKWPushButton* SelButton2;
  vtkKWPushButton* SelButton3;

private:
  vtkPVGhostLevelDialog(const vtkPVGhostLevelDialog&); // Not implemented
  void operator=(const vtkPVGhostLevelDialog&); // Not implemented
};


#endif


