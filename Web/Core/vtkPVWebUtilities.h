/*=========================================================================

  Program:   ParaView
  Module:    $RCSfile$

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVWebUtilities - collection of utility functions for ParaView Web.
// .SECTION Description
// vtkPVWebUtilities consolidates miscellaneous utility functions useful for
// Python scripts designed for ParaView Web.

#ifndef __vtkPVWebUtilities_h
#define __vtkPVWebUtilities_h

#include "vtkObject.h"
#include "vtkParaViewWebCoreModule.h" // needed for exports
#include <string>

class vtkDataSet;

class VTKPARAVIEWWEBCORE_EXPORT vtkPVWebUtilities : public vtkObject
{
public:
  static vtkPVWebUtilities* New();
  vtkTypeMacro(vtkPVWebUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  static std::string WriteAttributesToJavaScript(int field_type, vtkDataSet*);
  static std::string WriteAttributeHeadersToJavaScript(
    int field_type, vtkDataSet*);
//BTX
protected:
  vtkPVWebUtilities();
  ~vtkPVWebUtilities();

private:
  vtkPVWebUtilities(const vtkPVWebUtilities&); // Not implemented
  void operator=(const vtkPVWebUtilities&); // Not implemented
//ETX
};

#endif
// VTK-HeaderTest-Exclude: vtkPVWebUtilities.h
