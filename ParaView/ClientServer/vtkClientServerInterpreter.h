/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerInterpreter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkClientServerInterpreter - Run-time VTK interpreter.
// .SECTION Description
// vtkClientServerInterpreter will process messages stored in a
// vtkClientServerStream.  This allows run-time creation and execution
// of VTK programs.

#ifndef __vtkClientServerInterpreter_h
#define __vtkClientServerInterpreter_h

#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed for vtkClientServerID.

//BTX
template<class KeyType, class DataType> class vtkHashMap;
//ETX

class vtkClientServerInterpreter;
class vtkClientServerInterpreterCommand;
class vtkClientServerInterpreterInternals;
class vtkClientServerStream;
class vtkTimerLog;

// Description:
// The type of a command function.  One such function is generated per
// class wrapped.  It knows how to call the methods for that class at
// run-time.
typedef int (*vtkClientServerCommandFunction)(vtkClientServerInterpreter*,
                                              vtkObjectBase* ptr,
                                              const char* method,
                                              const vtkClientServerStream& msg,
                                              vtkClientServerStream& result);

// Description:
// The type of a new-instance function.  One such function is
// generated per library of wrappers.  It knows how to create
// instances of any of the classes included in the library.
typedef int (*vtkClientServerNewInstanceFunction)(vtkClientServerInterpreter*,
                                                  const char* name,
                                                  vtkClientServerID id);

class VTK_CLIENT_SERVER_EXPORT vtkClientServerInterpreter : public vtkObject
{
public:
  static vtkClientServerInterpreter* New();
  vtkTypeRevisionMacro(vtkClientServerInterpreter, vtkObject);

  // Description:
  // Process all messages in a given vtkClientServerStream.  Return 1
  // if all messages succeeded, and 0 otherwise.
  int ProcessStream(const unsigned char* msg, size_t msgLength);
  int ProcessStream(const vtkClientServerStream& css);

  // Description:
  // Process the message with the given index in the given stream.
  // Returns 1 for success, 0 for failure.
  int ProcessOneMessage(const vtkClientServerStream& css, int message);

  // Description:
  // Get the message for an ID.  ID 0 always returns a NULL message.
  const vtkClientServerStream* GetMessageFromID(vtkClientServerID id);

  // Description:
  // Get the last result message.
  const vtkClientServerStream& GetLastResult() const;

  // Description:
  // Return a pointer to a vtkObjectBase for an ID whose message
  // contains only the one object.
  vtkObjectBase* GetObjectFromID(vtkClientServerID id);

  // Description:
  // Return an ID given a pointer to a vtkObjectBase (or 0 if object
  // is not found)
  vtkClientServerID GetIDFromObject(vtkObjectBase* key);

  // Description:
  // Get/Set a stream to which an execution log is written.
  void SetLogFile(const char* name);
  virtual void SetLogStream(ostream* ostr);
  vtkGetMacro(LogStream, ostream*);

  // Description:
  // Called by generated code to register a new class instance.  Do
  // not call directly.
  int NewInstance(vtkObjectBase* obj, vtkClientServerID id);

  // Description:
  // Called by generated code to add an observer to a wrapped object.
  // Do not call directly.
  int NewObserver(vtkObject* obj, const char* event,
                  const vtkClientServerStream& css);

  // Description:
  // Add a command function for a class.
  void AddCommandFunction(const char* cname,
                          vtkClientServerCommandFunction func);

  // Description:
  // Get the command function for an object's class.
  vtkClientServerCommandFunction GetCommandFunction(vtkObjectBase* obj);

  // Description:
  // Add a function used to create new objects.
  void AddNewInstanceFunction(vtkClientServerNewInstanceFunction f);

  // Description:
  // The callback data structure passed to observers looking for VTK
  // object creation and deletion events.
  struct NewCallbackInfo
  {
    const char* Type;
    unsigned long ID;
  };

protected:
  // Map from ID to message stream.
  typedef vtkHashMap<vtkTypeUInt32, vtkClientServerStream*> IDToMessageMapType;
  IDToMessageMapType *IDToMessageMap;

  // Map from class name to command function.
  typedef vtkHashMap<const char*, vtkClientServerCommandFunction>
          ClassToFunctionMapType;
  ClassToFunctionMapType* ClassToFunctionMap;

  // constructor and destructor
  vtkClientServerInterpreter();
  ~vtkClientServerInterpreter();

  // A stream to which a log is written.
  ostream* LogStream;
  ofstream* LogFileStream;

  // Internal message processing functions.
  int ProcessCommandNew(const vtkClientServerStream& css, int midx);
  int ProcessCommandInvoke(const vtkClientServerStream& css, int midx);
  int ProcessCommandDelete(const vtkClientServerStream& css, int midx);
  int ProcessCommandAssign(const vtkClientServerStream& css, int midx);

  // Expand all the id_value arguments of a message starting with the
  // given argument index.
  int ExpandMessage(const vtkClientServerStream& in, int inIndex,
                    int startArgument, vtkClientServerStream& out);

private:

  // Message containing the result of the last command.
  vtkClientServerStream* LastResultMessage;

  // Internal implementation details.
  vtkClientServerInterpreterInternals* Internal;

  friend class vtkClientServerInterpreterCommand;
private:
  vtkClientServerInterpreter(const vtkClientServerInterpreter&);  // Not implemented.
  void operator=(const vtkClientServerInterpreter&);  // Not implemented.
};

#endif
