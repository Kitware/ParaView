/*=========================================================================

  Module:    vtkKWBWidgets.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWBWidgets - class used to initialized bwidgets
// .SECTION Description
// This class is used to initialize the bwidgets used by
// KW widgets.


#ifndef __vtkKWBWidgets_h
#define __vtkKWBWidgets_h

#include "vtkObject.h"
#include "vtkKWWidgets.h" // Needed for export symbols directives
#include "vtkTcl.h" // Needed for Tcl interpreter

class KWWIDGETS_EXPORT vtkKWBWidgets : public vtkObject
{
public:
  static vtkKWBWidgets* New();
  vtkTypeRevisionMacro(vtkKWBWidgets,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Load the bwidgets.
  static void Initialize(Tcl_Interp*);

protected:
  vtkKWBWidgets() {};
  ~vtkKWBWidgets() {};

  static void Execute(Tcl_Interp* interp, 
                      const unsigned char *buffer, 
                      unsigned long length,
                      unsigned long decoded_length);

private:
  vtkKWBWidgets(const vtkKWBWidgets&);   // Not implemented.
  void operator=(const vtkKWBWidgets&);  // Not implemented.
};

#endif
