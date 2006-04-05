/*=========================================================================

  Module:    vtkKWMyBlueTheme.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWMyBlueTheme - a theme class
// .SECTION Description
// .SECTION Thanks
// This work is part of the National Alliance for Medical Image
// Computing (NAMIC), funded by the National Institutes of Health
// through the NIH Roadmap for Medical Research, Grant U54 EB005149.
// Information on the National Centers for Biomedical Computing
// can be obtained from http://nihroadmap.nih.gov/bioinformatics.

#ifndef __vtkKWMyBlueTheme_h
#define __vtkKWMyBlueTheme_h

#include "vtkKWTheme.h"

class vtkKWMyBlueTheme : public vtkKWTheme
{
public:
  static vtkKWMyBlueTheme* New();
  vtkTypeRevisionMacro(vtkKWMyBlueTheme, vtkKWTheme);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Ask the theme to install itself
  virtual void Install();

protected:
  vtkKWMyBlueTheme() {};
  ~vtkKWMyBlueTheme() {};

private:

  vtkKWMyBlueTheme(const vtkKWMyBlueTheme&); // Not implemented
  void operator=(const vtkKWMyBlueTheme&); // Not implemented
};

#endif
