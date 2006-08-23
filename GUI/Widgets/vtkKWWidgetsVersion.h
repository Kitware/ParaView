/*=========================================================================

  Module:    vtkKWWidgetsVersion.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWWidgetsVersion - Versioning class for KWWidgets
// .SECTION Description
// Holds methods for defining/determining the current KWWidgets version
// (major, minor, build).
// .SECTION Caveats
// This file will change frequently to update the VTK_SOURCE_VERSION which
// timestamps a particular source release.

#ifndef __vtkKWWidgetsVersion_h
#define __vtkKWWidgetsVersion_h

#include "vtkKWWidgets.h" // Needed for export symbols directives
#include "vtkObject.h"

#define KWWidgets_SOURCE_VERSION "KWWidgets version " KWWidgets_VERSION ", KWWidgets source $Revision: 1.35 $, $Date: 2006-08-23 06:20:38 $ (GMT)"

class KWWidgets_EXPORT vtkKWWidgetsVersion : public vtkObject
{
public:
  static vtkKWWidgetsVersion *New();
  vtkTypeRevisionMacro(vtkKWWidgetsVersion,vtkObject);

  // Description: 
  // Return the major, minor and patch version of the KWWidgets library.
  static int GetKWWidgetsMajorVersion() 
    { return KWWidgets_MAJOR_VERSION; }
  static int GetKWWidgetsMinorVersion() 
    { return KWWidgets_MINOR_VERSION; }
  static int GetKWWidgetsPatchVersion() 
    { return KWWidgets_PATCH_VERSION; }

  // Description: 
  // Return the version of the KWWidgets library (as an aggregation of
  // the major, minor and patch version).
  static const char *GetKWWidgetsVersion() 
    { return KWWidgets_VERSION; }

  // Description: 
  // Return a string with an identifier which timestamps a particular source
  // tree. 
  static const char *GetKWWidgetsSourceVersion() 
    { return KWWidgets_SOURCE_VERSION; }
  
protected:
  vtkKWWidgetsVersion() {};
  ~vtkKWWidgetsVersion() {};

private:
  vtkKWWidgetsVersion(const vtkKWWidgetsVersion&);  // Not implemented.
  void operator=(const vtkKWWidgetsVersion&);  // Not implemented.
};

#endif 
