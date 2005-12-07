/*=========================================================================

  Module:    vtkKWBWidgetsInit.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWBWidgetsInit - class used to initialize BWidgets
// .SECTION Description
// This class is used to initialize the BWidgets library.

#ifndef __vtkKWBWidgetsInit_h
#define __vtkKWBWidgetsInit_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives
#include "vtkTcl.h" // Needed for Tcl interpreter

class KWWidgets_EXPORT vtkKWBWidgetsInit : public vtkObject
{
public:
  static vtkKWBWidgetsInit* New();
  vtkTypeRevisionMacro(vtkKWBWidgetsInit,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the BWidgets library.
  static void Initialize(Tcl_Interp*);

protected:
  vtkKWBWidgetsInit() {};
  ~vtkKWBWidgetsInit() {};

  static void Execute(Tcl_Interp* interp, 
                      const unsigned char *buffer, 
                      unsigned long length,
                      unsigned long decoded_length);

  static int Initialized;

private:
  vtkKWBWidgetsInit(const vtkKWBWidgetsInit&);   // Not implemented.
  void operator=(const vtkKWBWidgetsInit&);  // Not implemented.
};

#endif
