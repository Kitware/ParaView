/*=========================================================================

  Module:    vtkKWObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWObject - Superclass that supports basic Tcl functionality
// .SECTION Description
// vtkKWObject is the superclass for most application classes.
// It is a direct subclass of vtkObject but adds functionality for 
// invoking Tcl scripts, obtaining the Tcl name for an instance, etc. 
// This class requires a vtkKWApplication in order to work (as do all classes).
// .SECTION See Also
// vtkKWApplication

#ifndef __vtkKWObject_h
#define __vtkKWObject_h

#include "vtkObject.h"

// This has to be here because on HP varargs are included in 
// tcl.h and they have different prototypes for va_start so
// the build fails. Defining HAS_STDARG prevents that.

#if defined(__hpux) && !defined(HAS_STDARG)
#define HAS_STDARG
#endif

#include "vtkTcl.h" // Needed for Tcl interpreter
#include <stdarg.h> // Needed for "va_list" argument of EstimateFormatLength.

class vtkKWApplication;

class VTK_EXPORT vtkKWObject : public vtkObject
{
public:
  static vtkKWObject* New();
  vtkTypeRevisionMacro(vtkKWObject,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the name of the Tcl object this instance represents.
  const char *GetTclName();

  // Description:
  // Set/Get the application instance for this object.
  vtkGetObjectMacro(Application,vtkKWApplication);
  virtual void SetApplication (vtkKWApplication* arg);

  //BTX
  // Description:
  // Convenience method to invoke some Tcl script code and
  // perform argument substitution.
  virtual const char* Script(const char *EventString, ...);
  //ETX
  
protected:
  vtkKWObject();
  ~vtkKWObject();

  // Description:
  // Instance variable that holds the command function for this class.
  //BTX
  int (*CommandFunction)(ClientData, Tcl_Interp *, int, char *[]);
  //ETX

  // Description:
  // Convenience static method that can be used to create a callback function
  // on an object. The first argument is the command (string) to set, the 
  // second is the KWObject that the method will be called on. The third is
  // the name of the method itself and any arguments in string form. 
  // Note that the command is allocated automatically using the 'new' 
  // operator. If it is not NULL, it is deallocated first using 'delete []'.
  static void SetObjectMethodCommand(
    char **command, vtkKWObject *object, const char *method);

private:

  vtkKWApplication *Application;
  char *TclName;

  vtkKWObject(const vtkKWObject&); // Not implemented
  void operator=(const vtkKWObject&); // Not implemented
};

#endif

