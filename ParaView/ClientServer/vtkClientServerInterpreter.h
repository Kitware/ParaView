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

#include "vtkClientServerStream.h" // Needed for vtkClientServerID
#include "vtkSmartPointer.h" // Needed for InformationMap

#include <vector> // Needed for inlined container
#include <map> // Needed for inlined container
#include <string> // Needed for inlined container

#ifdef WIN32
#define VTKClientServer_EXPORT __declspec( dllexport )
#else
#define VTKClientServer_EXPORT
#endif

class vtkClientServerMessage;
class vtkClientServerInterpreter;
class vtkClientServerStream;
class vtkTimerLog;

//BTX
template<class KeyType, class DataType> class vtkHashMap;
//ETX

typedef int (*vtkClientServerCommandFunction)(vtkClientServerInterpreter *,
                                              vtkObjectBase *ptr, const char *method, 
                                              vtkClientServerMessage *msg,
                                              vtkClientServerStream*);

typedef int (*vtkClientServerNewInstanceFunction)(vtkClientServerInterpreter*,
                                                  const char* name,
                                                  vtkClientServerID id);

class VTK_EXPORT vtkClientServerInterpreter : public vtkObject
{
public:
  
  vtkTypeRevisionMacro(vtkClientServerInterpreter, vtkObject);

  // Description:
  static vtkClientServerInterpreter *New();

  // expand all the id_value arguments of a message
  vtkClientServerMessage *ExpandMessage(vtkClientServerMessage *msg);
  
  // return a message for an ID, handle the special zero ID
  vtkClientServerMessage *GetMessageFromID(vtkClientServerID id);
    
  // return an pointer to a vtkObject for a message argument
  vtkObject *GetObjectFromMessage(vtkClientServerMessage *msg, int num, int verbose);
  
  // get a vtk object * for an id
  vtkObject *GetObjectFromID(vtkClientServerID id);

  // return an ID given a pointer to a vtkObject (or 0 if
  // object is not found)
  unsigned long GetIDFromObject(vtkObject *key);

  // add a command function for a class
  void AddCommandFunction(const char *cname, vtkClientServerCommandFunction func);
  
  // get a command function for a vtkObject *
  vtkClientServerCommandFunction GetCommandFunction(vtkObject *ptr);
  
  // get an argument as a string, you must delete [] the result
  static char *GetString(vtkClientServerMessage *msg, int argNum);

  // process a message from a ClientServerStream
  int ProcessMessage(const unsigned char* msg, size_t msgLength);
  int ProcessMessage(vtkClientServerStream *str);
  int ProcessOneMessage(vtkClientServerMessage *msg);

  // assign the last result to an ID
  int AssignResultToID(vtkClientServerMessage *msg);
  
  // invoke a method on an existing instance
  int InvokeMethod(vtkClientServerMessage *msg);

  // create a new instance and setup the hash table entries
  int NewInstance(vtkObjectBase *ptr, vtkClientServerID id);
  int NewValue(vtkClientServerMessage *ptr, vtkClientServerID id);
  
  // delete an existing instance 
  int DeleteValue(vtkClientServerMessage *msg);  
  
  // Set the function used to create new objects
  void AddNewInstanceFunction(vtkClientServerNewInstanceFunction f) 
    {this->NewInstanceFunctions.push_back(f);}

  struct NewCallbackInfo
  {
    const char* Type;
    unsigned long ID;
  };
protected:
  vtkHashMap<unsigned long, vtkClientServerMessage *> *IDToMessageMap;
  vtkHashMap<const char *, vtkClientServerCommandFunction> *ClassToFunctionMap;

  // constructor and destructor
  vtkClientServerInterpreter();
  ~vtkClientServerInterpreter();

  unsigned char* MessageBuffer;

  void AllocateMessageBuffer(vtkIdType len);

  vtkTimerLog* ServerProgressTimer;
  vtkTimerLog* ClientProgressTimer;

private:
  vtkClientServerMessage *LastResultMessage;

  vtkClientServerInterpreter(const vtkClientServerInterpreter&);  // Not implemented.
  void operator=(const vtkClientServerInterpreter&);  // Not implemented.

  vtkIdType MessageBufferSize;
  vtkstd::vector<vtkClientServerNewInstanceFunction> NewInstanceFunctions;
};

#endif
