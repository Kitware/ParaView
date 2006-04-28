/*=========================================================================

  Module:    vtkKWTheme.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTheme - a theme superclass
// .SECTION Description
// This class provides very simple/basic theming capabilities. 
// It is under development right now.
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWTheme_h
#define __vtkKWTheme_h

#include "vtkKWObject.h"

class vtkKWOptionDataBase;

class KWWidgets_EXPORT vtkKWTheme : public vtkKWObject
{
public:
  static vtkKWTheme* New();
  vtkTypeRevisionMacro(vtkKWTheme, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Ask the theme to install/uninstall itself.
  // Subclasses should make sure to call the same superclass methods before
  // setting up their own options so that the application's option
  // database is backup'ed/restored correctly.
  virtual void Install();
  virtual void Uninstall();

protected:
  vtkKWTheme();
  ~vtkKWTheme();

  // Description:
  // Backup the current option-database, and restore it
  virtual void BackupCurrentOptionDataBase();
  virtual void RestorePreviousOptionDataBase();

  vtkKWOptionDataBase *BackupOptionDataBase;

private:

  vtkKWTheme(const vtkKWTheme&); // Not implemented
  void operator=(const vtkKWTheme&); // Not implemented
};

#endif
