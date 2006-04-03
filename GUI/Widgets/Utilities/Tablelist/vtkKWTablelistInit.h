/*=========================================================================

  Module:    vtkKWTablelistInit.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTablelistInit - class used to initialize Tablelist
// .SECTION Description
// This class is used to initialize the Tablelist library.

#ifndef __vtkKWTablelistInit_h
#define __vtkKWTablelistInit_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives
#include "vtkTcl.h"       // Needed for Tcl interpreter

class KWWidgets_EXPORT vtkKWTablelistInit : public vtkObject
{
public:
  static vtkKWTablelistInit* New();
  vtkTypeRevisionMacro(vtkKWTablelistInit,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the Tablelist library.
  static void Initialize(Tcl_Interp*);

protected:
  vtkKWTablelistInit() {};
  ~vtkKWTablelistInit() {};

  static void Execute(Tcl_Interp* interp, 
                      const unsigned char *buffer, 
                      unsigned long length,
                      unsigned long decoded_length);

  static int Initialized;

private:
  vtkKWTablelistInit(const vtkKWTablelistInit&);   // Not implemented.
  void operator=(const vtkKWTablelistInit&);  // Not implemented.
};

#endif
