/*=========================================================================

  Module:    vtkPVTraceHelper.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPVTraceHelper - trace routines
// .SECTION Description
// vtkPVTraceHelper provides trace features for the PV framework.

#ifndef __vtkPVTraceHelper_h
#define __vtkPVTraceHelper_h

#include "vtkObject.h"
#include <stdarg.h> // Needed for "va_list" argument 

class vtkKWObject;

class VTK_EXPORT vtkPVTraceHelper : public vtkObject
{
public:
  static vtkPVTraceHelper* New();
  vtkTypeRevisionMacro(vtkPVTraceHelper, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the object (Object) being traced.
  // Note: Object is not reference-counted at the moment.
  virtual void SetObject(vtkKWObject *);
  vtkGetObjectMacro(Object, vtkKWObject);
  
  // Description:
  // If needed, a unique name for Object in the trace context.
  vtkSetStringMacro(ObjectName);
  vtkGetStringMacro(ObjectName);

  // Description:
  // This variable is used to determine who set the trace ObjectName of Object.
  // Initially, the ObjectName is Uninitialized. Then,
  // usually, vtkPVXMLPackageParser assigns a Default name. Then,
  // either it is set from XML, the user sets it or one is assigned 
  // during some other methods (like Create()).
  vtkSetMacro(ObjectNameState,int);
  vtkGetMacro(ObjectNameState,int);
  //BTX
  enum ObjectNameState_t
  {
    ObjectNameStateUninitialized,
    ObjectNameStateDefault,
    ObjectNameStateXMLInitialized,
    ObjectNameStateSelfInitialized,
    ObjectNameStateUserInitialized
  };
  //ETX

  // Description:
  // Set/Get the reference trace helper (ReferenceHelper) and the 
  // initialization command (ReferenceCommand). This will allow the object
  // being traced (Object) to be initialized/created in the trace when
  // necessary, by recusively calling the command on the reference helper's
  // Object: this should return the object being traced (Object). 
  // Note: ReferenceHelper is not reference counted at the moment.
  virtual void SetReferenceHelper(vtkPVTraceHelper *);
  vtkGetObjectMacro(ReferenceHelper, vtkPVTraceHelper);
  vtkSetStringMacro(ReferenceCommand);
  vtkGetStringMacro(ReferenceCommand);

  // Description:
  // Initialize the trace.
  // Returns 1 if the trace for Object has been initialized. 
  // If it has not, it tries to initialize the object
  // by invoking an event. If this does not work, it returns 0.
  // The argument is used to save a trace into a state file.
  // When NULL or without argument, the Object's application trace file
  // is used (convenience).
  virtual int Initialize(ofstream* file);
  virtual int Initialize();

  // Description:
  // If a callback initializes Object, it can indicate so by setting this flag.
  vtkSetMacro(Initialized, int);
  vtkGetMacro(Initialized, int);
  vtkBooleanMacro(Initialized, int);

  // Description:
  // Convenience method to get the trace file associated to the Object's
  // application, if any. Return NULL if Object's is not defined, has
  // no application, or no trace file.
  ofstream* GetFile();

  // Description:
  // Convenience method that initializes the trace for Object and and handles
  // formating the tracecommand to Object's application's trace file (i.e,
  // Object has to be set, and its application should be set to). 
  // The formated string should contain a command that looks like:
  // "$kw(%s) SetValue %d", this->GetTclName(), this->GetValue().  
  virtual void AddSimpleEntry(const char *trace);
  //BTX
  virtual void AddEntry(const char *format, ...);
  //ETX

  // Description:
  // Convenience *static* method that handles formating the trace command
  // to a stream.
  // The formated string should contain a command that looks like:
  // "$kw(%s) SetValue %d", this->GetTclName(), this->GetValue().  
  static void OutputSimpleEntry(ostream *os, const char *trace);
  //BTX
  static void OutputEntry(ostream *os, const char *format, ...);
  //ETX

protected:
  vtkPVTraceHelper();
  ~vtkPVTraceHelper();

  // Description:
  // This flag indicates that a variable has been defined in the 
  // trace file for the object being traced.
  int Initialized;

  // Description:
  // Unique trace name
  char *ObjectName;
  int ObjectNameState;

  // Description:
  // The object being traced
  vtkKWObject *Object;

  // Description:
  // The object being traced can be obtained from the reference helper
  // in the trace script.
  vtkPVTraceHelper *ReferenceHelper;

  // Description:
  // The method and args that can be used to get the object being traced
  // (Object) from the reference helper's Object (ReferenceHelper).
  char *ReferenceCommand;

  // Description:
  // Internal method
  static void OutputEntryInternal(
    ostream *os, int estimated_length, const char *format, va_list ap);

private:

  vtkPVTraceHelper(const vtkPVTraceHelper&); // Not implemented
  void operator=(const vtkPVTraceHelper&); // Not implemented
};

#endif

