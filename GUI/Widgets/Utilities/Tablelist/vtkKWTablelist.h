/*=========================================================================

  Module:    vtkKWTablelist.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWTablelist - class used to initialize Tablelist
// .SECTION Description
// This class is used to initialize the Tablelist library.

#ifndef __vtkKWTablelist_h
#define __vtkKWTablelist_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives
#include "vtkTcl.h" // Needed for Tcl interpreter

class KWWIDGETS_EXPORT vtkKWTablelist : public vtkObject
{
public:
  static vtkKWTablelist* New();
  vtkTypeRevisionMacro(vtkKWTablelist,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the Tablelist library.
  static void Initialize(Tcl_Interp*);

protected:
  vtkKWTablelist() {};
  ~vtkKWTablelist() {};

  static void Execute(Tcl_Interp* interp, 
                      const unsigned char *buffer, 
                      unsigned long length,
                      unsigned long decoded_length);

  static int Initialized;

private:
  vtkKWTablelist(const vtkKWTablelist&);   // Not implemented.
  void operator=(const vtkKWTablelist&);  // Not implemented.
};

#endif
