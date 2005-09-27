/*=========================================================================

  Module:    vtkKWTkconInit.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTkconInit - class used to initialize Tkcon
// .SECTION Description
// This class is used to initialize the Tkcon library.

#ifndef __vtkKWTkconInit_h
#define __vtkKWTkconInit_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives
#include "vtkTcl.h" // Needed for Tcl interpreter

class KWWIDGETS_EXPORT vtkKWTkconInit : public vtkObject
{
public:
  static vtkKWTkconInit* New();
  vtkTypeRevisionMacro(vtkKWTkconInit,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the Tkcon library.
  static void Initialize(Tcl_Interp*);
  static int GetInitialized() { return vtkKWTkconInit::Initialized; }

protected:
  vtkKWTkconInit() {};
  ~vtkKWTkconInit() {};

  static void Execute(Tcl_Interp* interp, 
                      const unsigned char *buffer, 
                      unsigned long length,
                      unsigned long decoded_length);

  static int Initialized;

private:
  vtkKWTkconInit(const vtkKWTkconInit&);   // Not implemented.
  void operator=(const vtkKWTkconInit&);  // Not implemented.
};

#endif
