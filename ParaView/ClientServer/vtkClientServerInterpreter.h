/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerInterpreter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1998-1999 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/
#ifndef __vtkClientServerInclude_h
#define __vtkClientServerInclude_h

#include "vtkObject.h"

#include "vtkClientServerID.h" // Needed for vtkClientServerID.

#ifdef WIN32
#define VTKClientServer_EXPORT __declspec( dllexport )
#else
#define VTKClientServer_EXPORT
#endif

class vtkClientServerInterpreter;
class vtkClientServerInterpreterInternals;
class vtkClientServerStream;
class vtkTimerLog;

//BTX
template<class KeyType, class DataType> class vtkHashMap;
//ETX

typedef int (*vtkClientServerCommandFunction)(vtkClientServerInterpreter*,
                                              vtkObjectBase* ptr,
                                              const char* method, 
                                              const vtkClientServerStream& msg,
                                              vtkClientServerStream& result);

typedef int (*vtkClientServerNewInstanceFunction)(vtkClientServerInterpreter*,
                                                  const char* name,
                                                  vtkClientServerID id);

class VTK_EXPORT vtkClientServerInterpreter : public vtkObject
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
  // Get the message for an ID.  Special id 0 retrieves the result of
  // the last command.
  const vtkClientServerStream* GetMessageFromID(vtkClientServerID id);
  
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
  vtkSetMacro(LogStream, ostream*);
  vtkGetMacro(LogStream, ostream*);  
  
  // Description:
  // Called by generated code to register a new class instance.  Do
  // not call directly.
  int NewInstance(vtkObjectBase* obj, vtkClientServerID id);
  
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

  vtkTimerLog* ServerProgressTimer;
  vtkTimerLog* ClientProgressTimer;
  
  // A stream to which a log is written.
  ostream* LogStream;
  
  // Internal message processing functions.
  int ProcessCommandNew(const vtkClientServerStream& css, int midx);
  int ProcessCommandInvoke(const vtkClientServerStream& css, int midx);
  int ProcessCommandDelete(const vtkClientServerStream& css, int midx);
  int ProcessCommandAssignResult(const vtkClientServerStream& css, int midx);

  // Expand all the id_value arguments of a message.
  int ExpandMessage(const vtkClientServerStream& in, int inIndex,
                    vtkClientServerStream& out);
  
  // Assign the last result to the given ID.  Returns 1 for success, 0
  // for failure.
  int AssignResultToID(vtkClientServerID id);

private:
  
  // Message containing the result of the last command.
  vtkClientServerStream* LastResultMessage;
  
  // Internal implementation details.
  vtkClientServerInterpreterInternals* Internal;

private:
  vtkClientServerInterpreter(const vtkClientServerInterpreter&);  // Not implemented.
  void operator=(const vtkClientServerInterpreter&);  // Not implemented.
};

#endif
