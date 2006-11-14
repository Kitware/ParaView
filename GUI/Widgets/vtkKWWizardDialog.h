/*=========================================================================

  Module:    vtkKWWizardDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWizardDialog - a superclass for creating wizards UI.
// .SECTION Description
// This class is the basis for a wizard widget/dialog. This dialog
// is a thin toplevel wrapper embedding a vtkKWWizardWidget.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWWizardStep vtkKWWizardWorkflow

#ifndef __vtkKWWizardDialog_h
#define __vtkKWWizardDialog_h

#include "vtkKWDialog.h"

class vtkKWWizardWorkflow;
class vtkKWWizardWidget;

class KWWidgets_EXPORT vtkKWWizardDialog : public vtkKWDialog
{
public:
  static vtkKWWizardDialog* New();
  vtkTypeRevisionMacro(vtkKWWizardDialog,vtkKWDialog);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the internal wizard widget.
  vtkGetObjectMacro(WizardWidget, vtkKWWizardWidget);

  // Description:
  // Convenience method to get the wizard widget's wizard workflow.
  virtual vtkKWWizardWorkflow* GetWizardWorkflow();

  // Description:
  // Update the "enable" state of the object and its internal parts.
  // Depending on different Ivars (this->Enabled, the application's 
  // Limited Edition Mode, etc.), the "enable" state of the object is updated
  // and propagated to its internal parts/subwidgets. This will, for example,
  // enable/disable parts of the widget UI, enable/disable the visibility
  // of 3D widgets, etc.
  virtual void UpdateEnableState();

protected:
  vtkKWWizardDialog();
  ~vtkKWWizardDialog();

  // Description:
  // Create the widget
  virtual void CreateWidget();

  vtkKWWizardWidget *WizardWidget;

private:
  vtkKWWizardDialog(const vtkKWWizardDialog&); // Not implemented
  void operator=(const vtkKWWizardDialog&); // Not Implemented
};

#endif
