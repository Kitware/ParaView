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
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWTheme_h
#define __vtkKWTheme_h

#include "vtkKWObject.h"

class KWWidgets_EXPORT vtkKWTheme : public vtkKWObject
{
public:
  static vtkKWTheme* New();
  vtkTypeRevisionMacro(vtkKWTheme, vtkKWObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Ask the theme to install/uninstall itself
  virtual void Install();
  virtual void Uninstall();

protected:
  vtkKWTheme() {};
  ~vtkKWTheme() {};

private:

  vtkKWTheme(const vtkKWTheme&); // Not implemented
  void operator=(const vtkKWTheme&); // Not implemented
};

#endif
