/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkKWObject.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 2000-2001 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither the name of Kitware nor the names of any contributors may be used
   to endorse or promote products derived from this software without specific 
   prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

#include "tcl.h"

class vtkKWApplication;

class VTK_EXPORT vtkKWObject : public vtkObject
{
public:
  static vtkKWObject* New();
  vtkTypeMacro(vtkKWObject,vtkObject);
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
  // Chaining method to serialize an object and its superclasses.
  void Serialize(ostream& os, vtkIndent indent);
  virtual void SerializeRevision(ostream& os, vtkIndent indent);
  void Serialize(istream& is);
  virtual void SerializeSelf(ostream& /*os*/, vtkIndent /*indent*/) {};
  virtual void SerializeToken(istream& is, const char token[1024]);
  virtual const char *GetVersion(const char *);
  virtual void AddVersion(const char *cname, const char *version);
  void ExtractRevision(ostream& os,const char *revIn);
  int CompareVersions(const char *v1, const char *v2);
  
  // This method returns 1 if the trace for this object has been 
  // initialized. If it has not, it tries to initialize the object
  // by invoking an event.  If this does not work, it returns 0.
  virtual int InitializeTrace();

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
  void Script(const char *EventString, ...);

private:
  char *TclName;
  
protected:
  vtkKWObject();
  ~vtkKWObject();

  vtkKWApplication *Application;

  char **Versions;
  int   NumberOfVersions;
  int   VersionsLoaded;
  
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

private:
  vtkKWObject(const vtkKWObject&); // Not implemented
  void operator=(const vtkKWObject&); // Not implemented
};

#endif
