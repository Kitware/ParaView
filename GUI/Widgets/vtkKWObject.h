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

  // Description:
  // This method returns 1 if the trace for this object has been 
  // initialized. If it has not, it tries to initialize the object
  // by invoking an event.  If this does not work, it returns 0.
  // The argument is for saving a trace into a state file.
  // When NULL, then the application trace file is used (conveniance).
  // When file does not match global trace file, init flag is ignored.
  virtual int InitializeTrace(ofstream* file);

  // Description:
  // If the a callback initializes the widget, then it can indicate so
  // by setting this flag.
  vtkSetMacro(TraceInitialized, int);
  vtkGetMacro(TraceInitialized, int);
  vtkBooleanMacro(TraceInitialized, int);

  // Description:
  // Setting the reference object and its command will allow this
  // object to be initialized in the trace when necessary.
  // When command is called on the reference object, it should
  // return this object. Note:  We do not reference count
  // the reference object.  It could be done in the future.
  void SetTraceReferenceObject(vtkKWObject* o) {this->TraceReferenceObject = o;}
  vtkGetObjectMacro(TraceReferenceObject, vtkKWObject);
  vtkSetStringMacro(TraceReferenceCommand);
  vtkGetStringMacro(TraceReferenceCommand);

//BTX
  // Description:
  // A convienience method to invoke some tcl script code and
  // perform arguement substitution.
  virtual const char* Script(const char *EventString, ...);
  
  // Description:
  // Method to estimate the length of the string that will be produced
  // from printing the given format string and arguments.  The
  // returned length will always be at least as large as the string
  // that will result from printing.
  int EstimateFormatLength(const char* format, va_list ap);
  
private:
  char *TclName;
  
protected:
  vtkKWObject();
  ~vtkKWObject();

  // this instance variable holds the command functions for this class.
  int (*CommandFunction)(ClientData, Tcl_Interp *, int, char *[]);

  // Support for tracing activity to a script.
  // This flag indicates that a variable has been defined in the 
  // trace file for this widget.
  int TraceInitialized;
  // This object can be obtained from the reference object on the script.
  vtkKWObject *TraceReferenceObject;
  // This string is the method and args that can be used to get this object
  // from the reference object.
  char *TraceReferenceCommand;
  // Convenience method that initializes and handles formating the trace command.
  // The formated string should contain a command that looks like:
  // "$kw(%s) SetValue %d", this->GetTclName(), this->GetValue().  
  void AddTraceEntry(const char *EventString, ...);

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

