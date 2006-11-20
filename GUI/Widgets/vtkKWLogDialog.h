/*=========================================================================

  Module:    vtkKWLogDialog.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWLogDialog - a log dialog.
// .SECTION Description
// This widget can be used to display various types of records/events in the
// form of a multicolumn log. Each record is timestamped automatically, and 
/// the interface allow the user to sort the list by time, type, or 
// description.
// This dialog is a standalone toplevel, but uses a vtkKWLogWidget internally.
// The vtkKWLogWidget class can be inserted in any widget hierarchy.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.
// .SECTION See Also
// vtkKWLogWidget

#ifndef __vtkKWLogDialog_h
#define __vtkKWLogDialog_h

#include "vtkKWTopLevel.h"

class vtkKWApplication;
class vtkKWLogWidget;
class vtkKWPushButton;

class KWWidgets_EXPORT vtkKWLogDialog : public vtkKWTopLevel
{
public:
  static vtkKWLogDialog* New();
  vtkTypeRevisionMacro(vtkKWLogDialog,vtkKWTopLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the internal log widget so that its API will be exposed directly
  vtkGetObjectMacro(LogWidget, vtkKWLogWidget);   
   
protected:
  vtkKWLogDialog();
  ~vtkKWLogDialog();

  // Description:
  // Create the widget.
  virtual void CreateWidget();
  
  // Description:
  // Member variables
  vtkKWLogWidget* LogWidget;
  vtkKWPushButton* CloseButton;

private:
  vtkKWLogDialog(const vtkKWLogDialog&); // Not implemented
  void operator=(const vtkKWLogDialog&); // Not implemented
};

#endif
