/*=========================================================================

  Module:    vtkKWObject.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkKWObject - superclass that supports basic Tcl functionality
// .SECTION Description
// vtkKWObject is the superclass for most application classes.
// It is a direct subclass of vtkObject but adds functionality for 
// invoking Tcl scripts, obtains results from those scripts, and
// obtaining a Tcl name for an instance. This class requires a 
// vtkKWApplicaiton in order to work (as do all classes).

// .SECTION See Also
// vtkKWApplication

#ifndef __vtkKWObject_h
#define __vtkKWObject_h

#include "vtkObject.h"

// This has to be here because on HP varargs are included in 
// tcl.h and they have different prototypes for va_start so
// the build fails. Defining HAS_STDARG prevents that.
#if defined(__hpux) && !defined(HAS_STDARG)
#  define HAS_STDARG
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
  // Get the name of the tcl object this instance represents.
  const char *GetTclName();

  // Description:
  // Get the application instance for this class.
  vtkGetObjectMacro(Application,vtkKWApplication);
  virtual void SetApplication (vtkKWApplication* arg);

  // Description:
  // Convienience methods to get results of Tcl commands.
  static int GetIntegerResult(vtkKWApplication *);
  static float GetFloatResult(vtkKWApplication *);

  //BTX
  // Description:
  // A convienience method to invoke some tcl script code and
  // perform arguement substitution.
  virtual const char* Script(const char *EventString, ...);
  //ETX
  
private:
  char *TclName;
  
protected:
  vtkKWObject();
  ~vtkKWObject();

  // this instance variable holds the command functions for this class.
  //BTX
  int (*CommandFunction)(ClientData, Tcl_Interp *, int, char *[]);
  //ETX

  // This method can be used to create
  // A method to set a callback function on object.  The first argument is
  // the command (string) to set, the second is the KWObject that will have
  // the method called on it. The third is the name of the method to be
  // called and any arguments in string form. 
  // The calling is done via TCL wrappers for the KWObject.
  // If the command (string) is not NULL, it is deallocated first.
  virtual void SetObjectMethodCommand(
    char **command, vtkKWObject *object, const char *method);

private:

  vtkKWApplication *Application;

  vtkKWObject(const vtkKWObject&); // Not implemented
  void operator=(const vtkKWObject&); // Not implemented
};

#endif

