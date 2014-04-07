/*=========================================================================

  Program:   ParaView
  Module:    vtkSMCoreUtilities.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSMCoreUtilities - collection of utilities.
// .SECTION Description
// vtkSMCoreUtilities provides miscellaneous utility functions.

#ifndef __vtkSMCoreUtilities_h
#define __vtkSMCoreUtilities_h

#include "vtkPVServerManagerCoreModule.h" //needed for exports
#include "vtkObject.h"
#include "vtkStdString.h" // needed for vtkStdString.

class vtkSMProxy;

class VTKPVSERVERMANAGERCORE_EXPORT vtkSMCoreUtilities : public vtkObject
{
public:
  static vtkSMCoreUtilities* New();
  vtkTypeMacro(vtkSMCoreUtilities, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Given a proxy (or proxy prototype), returns the name of the property that
  // ParaView application will be use as the default FileName property.
  // Returns the name of the property or NULL when no such property is found.
  static const char* GetFileNameProperty(vtkSMProxy*);

  // Description:
  // Sanitize a label/name to be remove spaces, delimiters etc.
  static vtkStdString SanitizeName(const char*);

//BTX
protected:
  vtkSMCoreUtilities();
  ~vtkSMCoreUtilities();

private:
  vtkSMCoreUtilities(const vtkSMCoreUtilities&); // Not implemented
  void operator=(const vtkSMCoreUtilities&); // Not implemented
//ETX
};

#endif
