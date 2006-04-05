/*=========================================================================

  Module:    vtkKWMyGreenTheme.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMyGreenTheme - a theme class
// .SECTION Description
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWMyGreenTheme_h
#define __vtkKWMyGreenTheme_h

#include "vtkKWTheme.h"

class vtkKWMyGreenTheme : public vtkKWTheme
{
public:
  static vtkKWMyGreenTheme* New();
  vtkTypeRevisionMacro(vtkKWMyGreenTheme, vtkKWTheme);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Ask the theme to install itself
  virtual void Install();

protected:
  vtkKWMyGreenTheme() {};
  ~vtkKWMyGreenTheme() {};

private:

  vtkKWMyGreenTheme(const vtkKWMyGreenTheme&); // Not implemented
  void operator=(const vtkKWMyGreenTheme&); // Not implemented
};

#endif
