/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerInterpreter.cxx
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
#include "vtkClientServerInterpreter.h"

#include "vtkClientServerStream.h"
#include "vtkProcessObject.h"
#include "vtkVector.txx"
#include "vtkHashMap.txx"
#include "vtkObjectFactory.h"
#include "vtkCommand.h"
#include "vtkTimerLog.h"
#include "vtkDataSet.h"
#include "vtkSource.h"
#include "vtkClientServerMessage.h"

#include <numeric>

vtkStandardNewMacro(vtkClientServerInterpreter);
vtkCxxRevisionMacro(vtkClientServerInterpreter, "1.1.2.1");

template class vtkHashMap<unsigned long, vtkClientServerMessage *>;
template class vtkHashMap<const char *, vtkClientServerCommandFunction>;
template class vtkVector<unsigned char>;

static inline void vtkContainerDeleteMethod(vtkClientServerCommandFunction) {}
static inline vtkClientServerCommandFunction 
vtkContainerCreateMethod(vtkClientServerCommandFunction d1)
{ return vtkContainerDefaultCreate(d1); }


vtkClientServerInterpreter::vtkClientServerInterpreter()
{
  this->IDToMessageMap = vtkHashMap<unsigned long,vtkClientServerMessage *>::New();
  this->ClassToFunctionMap = 
    vtkHashMap<const char *,vtkClientServerCommandFunction>::New();
  this->LastResultMessage = 0;

  this->MessageBufferSize = 0;
  this->MessageBuffer = 0;
  this->AllocateMessageBuffer(10);
  this->ServerProgressTimer = vtkTimerLog::New();
  this->ClientProgressTimer = vtkTimerLog::New();
}

vtkClientServerInterpreter::~vtkClientServerInterpreter()
{
  // delete any remaining messages
  vtkHashMapIterator<unsigned long,vtkClientServerMessage *> *hi = 
    this->IDToMessageMap->NewIterator();
  vtkClientServerMessage *tmp;
  while (!hi->IsDoneWithTraversal())
    {
    hi->GetData(tmp);
    delete tmp;
    hi->GoToNextItem();
    }
  hi->Delete();
  
  this->IDToMessageMap->Delete();
  this->ClassToFunctionMap->Delete();
  if (this->LastResultMessage)
    {
    delete this->LastResultMessage;
    this->LastResultMessage = NULL;
    }

  delete[] this->MessageBuffer;

}

int vtkClientServerInterpreter::AssignResultToID(vtkClientServerMessage *msg)
{
  if (!this->LastResultMessage)
    {
    vtkGenericWarningMacro("Attempt to assign a result to an ID when there is no result");
    return 1;
    }
  
  // take the last result and assign it to an id
  vtkClientServerID id;
  id.ID = *reinterpret_cast<const unsigned int *>(msg->Arguments[0]);
  
  // copy the last result
  vtkClientServerMessage *newmsg = new vtkClientServerMessage;
  newmsg->Copy(this->LastResultMessage);
  
  // add the new entry to the hash table
  return this->NewValue(newmsg,id);
}

int vtkClientServerInterpreter::ProcessMessage(vtkClientServerStream *str)
{
  const unsigned char *msg;
  size_t msgLength;
  str->GetData(&msg,&msgLength);
  int retVal = this->ProcessMessage(msg, msgLength);
  return retVal;
}

int vtkClientServerInterpreter::ProcessMessage(const unsigned char* msg, size_t msgLength)
{
  int ret = 0;
  
  // break the message into pieces to be executed
  const unsigned char *pos = msg;
  const unsigned char *nextPos = msg;
  do
    {
    vtkClientServerMessage *amsg = vtkClientServerMessage::GetMessage(pos,msgLength - (pos - msg),
                                                    &nextPos);
    if (!amsg)
      {
      return 1;
      }
    ret = this->ProcessOneMessage(amsg);
    if (ret)
      {
      delete amsg;
      return ret;
      }
    pos = nextPos;
    delete amsg;
    }
  while (nextPos < msg+msgLength);
  
  return ret;
}

int vtkClientServerInterpreter::ProcessOneMessage(vtkClientServerMessage *amsg)
{
  int ret = 1;
  vtkDebugMacro("ProcessOneMessage: ");
  
  /* look for known messages */
  if (amsg->Command == vtkClientServerStream::New)
    {
    if(this->NewInstanceFunctions.size() == 0)
      {
      vtkGenericWarningMacro(
        "Attempt to use vtkClientServerInterpreter with no NewInstanceFunctions set");
      return ret;
      }

    if (amsg->NumberOfArguments == 2 &&
        amsg->ArgumentTypes[0] == vtkClientServerStream::string_value &&
        amsg->ArgumentTypes[1] == vtkClientServerStream::id_value)
      {
      char *type = vtkClientServerInterpreter::GetString(amsg,0);
      vtkClientServerID id;
      id.ID =
        *reinterpret_cast<const unsigned long *>(amsg->Arguments[1]);
      for(vtkstd::vector<vtkClientServerNewInstanceFunction>::iterator 
            it = this->NewInstanceFunctions.begin();
          it != this->NewInstanceFunctions.end(); ++it)
        {
        ret = (*(*it))(this, type, id);
        if(ret == 0)
          {
          break;
          }
        }
      if (ret)
        {
        vtkGenericWarningMacro("Attempt to create unsupported type " << type);
        }
      else
        {
        vtkClientServerInterpreter::NewCallbackInfo info;
        info.Type = type;
        info.ID = id.ID;
        this->InvokeEvent(vtkCommand::UserEvent+1, &info);
        }
      delete [] type;
      }
    return ret;
    }
  
  if (amsg->Command == vtkClientServerStream::Invoke)
    {
    if (amsg->NumberOfArguments >= 2 &&
        amsg->ArgumentTypes[0] == vtkClientServerStream::id_value &&
        amsg->ArgumentTypes[1] == vtkClientServerStream::string_value)
      {
      ret = this->InvokeMethod(amsg);
      }
    return ret;
    }
  
  if (amsg->Command == vtkClientServerStream::Delete)
    {
    if (amsg->NumberOfArguments == 1 &&
        amsg->ArgumentTypes[0] == vtkClientServerStream::id_value)
      {
      ret = this->DeleteValue(amsg);
      }
    return ret;
    }
  
  if (amsg->Command == vtkClientServerStream::AssignResult)
    {
    if (amsg->NumberOfArguments == 1 &&
        amsg->ArgumentTypes[0] == vtkClientServerStream::id_value)
      {
      ret = this->AssignResultToID(amsg);
      }
    return ret;
    }

  vtkGenericWarningMacro("Received unknown messgae type");
  return ret;
}

vtkClientServerMessage *vtkClientServerInterpreter::ExpandMessage(vtkClientServerMessage *msg)
{
  vtkClientServerMessage *amsg = new vtkClientServerMessage;
  amsg->NumberOfArguments = 0;
  amsg->Command = msg->Command;

  // find out how many arguments the expanded message will have
  unsigned int i;
  for (i = 0; i < msg->NumberOfArguments; ++i)
    {
    // is this argument an id?
    if (msg->ArgumentTypes[i] == vtkClientServerStream::id_value)
      {
      // look up the id to get the message
      vtkClientServerID id;
      id.ID = *(unsigned long *)msg->Arguments[i];
      vtkClientServerMessage *tmp = this->GetMessageFromID(id);
      if (tmp)
        {
        amsg->NumberOfArguments += tmp->NumberOfArguments;
        }
      else
        {
        vtkGenericWarningMacro("Tried to expand argument but failed to find id");
        amsg->NumberOfArguments++;
        }
      }
    else
      {
      amsg->NumberOfArguments++;
      }
    }
  
  // allocate storage and fill in the structure
  if (amsg->NumberOfArguments)
    {
    amsg->ArgumentTypes = 
      new vtkClientServerStream::Types [amsg->NumberOfArguments];
    amsg->ArgumentSizes = new unsigned int [amsg->NumberOfArguments];
    amsg->Arguments = 
      new const unsigned char * [amsg->NumberOfArguments];

    unsigned int count = 0;
    for (i = 0; i < msg->NumberOfArguments; ++i)
      {
      // is this argument an id?
      if (msg->ArgumentTypes[i] == vtkClientServerStream::id_value)
        {
        // look up the id to get the message
        vtkClientServerID id;
        id.ID = *(unsigned long *)msg->Arguments[i];
        vtkClientServerMessage *tmp = this->GetMessageFromID(id);
        if (tmp)
          {
          // copy over the arguments
          unsigned int j;
          for (j = 0; j < tmp->NumberOfArguments; ++j)
            {
            amsg->ArgumentTypes[count] = tmp->ArgumentTypes[j];
            amsg->ArgumentSizes[count] = tmp->ArgumentSizes[j];
            amsg->Arguments[count] = tmp->Arguments[j];
            count++;
            }
          }
        else
          {
          amsg->ArgumentTypes[count] = msg->ArgumentTypes[i];
          amsg->ArgumentSizes[count] = msg->ArgumentSizes[i];
          amsg->Arguments[count] = msg->Arguments[i];
          count++;
          }
        }
      else
        {
        amsg->ArgumentTypes[count] = msg->ArgumentTypes[i];
        amsg->ArgumentSizes[count] = msg->ArgumentSizes[i];
        amsg->Arguments[count] = msg->Arguments[i];
        count++;
        }
      }
    }
  
  return amsg;
}

int vtkClientServerInterpreter::NewValue(vtkClientServerMessage *ptr, vtkClientServerID id)
{
  // put the new instance into the hash tables
  // first make sure it isn't already there
  vtkClientServerMessage *tmp;
  if (this->IDToMessageMap->GetItem(id.ID,tmp) == VTK_OK)
    {
    vtkGenericWarningMacro("attempt to create an ID that is already in the hash table: " << id.ID);
    return 1;
    }
  this->IDToMessageMap->SetItem(id.ID,ptr);
  //cout << "NewValue(" << id.ID << ")" << endl;
  return 0;
}

int vtkClientServerInterpreter::NewInstance(vtkObjectBase *ptr, vtkClientServerID id)
{
  vtkClientServerMessage *msg = new vtkClientServerMessage;
  msg->NumberOfArguments = 1;
  msg->ArgumentTypes = new vtkClientServerStream::Types [1];
  msg->ArgumentSizes = new unsigned int [1];
  msg->Arguments = new const unsigned char *[1];
  const int pointerSize = sizeof(vtkObject *);
  msg->ArgumentTypes[0] = vtkClientServerStream::vtk_object_pointer;
  msg->ArgumentSizes[0] = pointerSize;
  msg->ArgumentData = new unsigned char [pointerSize];
  memcpy(msg->ArgumentData,&ptr,pointerSize);
  msg->Arguments[0] = msg->ArgumentData;
  return this->NewValue(msg,id);
}

int vtkClientServerInterpreter::DeleteValue(vtkClientServerMessage *msg)
{
  // find the info first
  unsigned long id = 
    *reinterpret_cast<const unsigned long *>(msg->Arguments[0]);
  
  // put the new instance into the hash tables
  // first make sure it isn;t already there
  vtkClientServerMessage *tmp;
  if (this->IDToMessageMap->GetItem(id,tmp) != VTK_OK)
    {
    vtkGenericWarningMacro("attempt to delete an ID that is not in the hash table"
                           << id);
    return 1;
    }
  
  // if it was a vtkObject then we must do a Delete
  if (tmp->NumberOfArguments > 0 && 
      tmp->ArgumentTypes[0] == vtkClientServerStream::vtk_object_pointer)
    {
    vtkObject *op = this->GetObjectFromMessage(tmp,0,1);
    if (!op)
      {
      vtkGenericWarningMacro("error in deleting a vtkObject");
      return 1;
      }

    vtkClientServerInterpreter::NewCallbackInfo info;
    info.Type = op->GetClassName();
    info.ID = id;
    this->InvokeEvent(vtkCommand::UserEvent+2, &info);

    op->Delete();
    }
  
  this->IDToMessageMap->RemoveItem(id);

  delete tmp;
  return 0;
}

void vtkClientServerInterpreter::AddCommandFunction(const char *cname, 
                                    vtkClientServerCommandFunction func)
{
  this->ClassToFunctionMap->SetItem(cname,func);
}

vtkObject *vtkClientServerInterpreter::GetObjectFromMessage(vtkClientServerMessage *msg, 
                                            int num,
                                            int verbose)
{
  if (msg->ArgumentTypes[num] == vtkClientServerStream::vtk_object_pointer)
    {
    return *(vtkObject **)(msg->Arguments[num]);
    }
  if (verbose)
    {
    vtkGenericWarningMacro("attempt to get an object for a bad message type");
    }
  return NULL;
}

unsigned long vtkClientServerInterpreter::GetIDFromObject(vtkObject *key)
{
  vtkHashMapIterator<unsigned long,vtkClientServerMessage *> *hi = 
    this->IDToMessageMap->NewIterator();
  vtkClientServerMessage *msg;
  unsigned long id = 0;
  while (!hi->IsDoneWithTraversal())
    {
    hi->GetData(msg);
    vtkObject* obj = this->GetObjectFromMessage(msg, 0, 0);
    if (obj == key)
      {
      hi->GetKey(id);
      break;
      }
    hi->GoToNextItem();
    }
  hi->Delete();
  return id;
}

vtkClientServerMessage *vtkClientServerInterpreter::GetMessageFromID(vtkClientServerID id)
{
  // look for special LastReturnMessage
  if (id.ID == 0)
    {
    return this->LastResultMessage;
    }
  
  vtkClientServerMessage *tmp;
  if (this->IDToMessageMap->GetItem(id.ID,tmp) != VTK_OK)
    {
    vtkGenericWarningMacro("attempt to get an ID that is not in the hash table: "
                           << id.ID);
    return 0;
    }
  return tmp;
}

vtkObject *vtkClientServerInterpreter::GetObjectFromID(vtkClientServerID id)
{
  vtkClientServerMessage *tmp = this->GetMessageFromID(id);
  if (!tmp)
    {
    vtkGenericWarningMacro("attempt to get an object for an ID that is not in the hash table" << id.ID);
    return NULL;
    }
  
  // verify the type
  if (tmp->NumberOfArguments != 1 || 
      tmp->ArgumentTypes[0] != vtkClientServerStream::vtk_object_pointer)
    {
    vtkGenericWarningMacro("attempt to get an object an ID that is not in the hash table" << id.ID);
    return NULL;
    }
  
  return *(vtkObject **)(tmp->Arguments[0]);
}

vtkClientServerCommandFunction vtkClientServerInterpreter::GetCommandFunction(vtkObject *obj)
{
  // look up the function
  vtkClientServerCommandFunction res = 0;
  const char *cname = obj->GetClassName();
  this->ClassToFunctionMap->GetItem(cname,res);
  if (!res)
    {
    vtkGenericWarningMacro("attempt to get a function that is not in the hash table");
    }
  return res;
}

int vtkClientServerInterpreter::InvokeMethod(vtkClientServerMessage *inmsg)
{
  // first expand the message
  vtkClientServerMessage *msg = this->ExpandMessage(inmsg);
  
  // get the function and pointer
  vtkObject *tmp = *(vtkObject **)(msg->Arguments[0]);
  if (!tmp || msg->ArgumentTypes[0] != vtkClientServerStream::vtk_object_pointer)
    {
    vtkGenericWarningMacro("attempt to invoke a method on an ID that is not in the hash table");
    delete msg;
    return 1;
    }

  vtkClientServerCommandFunction func = vtkClientServerInterpreter::GetCommandFunction(tmp);
  char *method = vtkClientServerInterpreter::GetString(msg,1);

  // now invoke the method
  const unsigned char *result = NULL;
  size_t resultLen = 0;
  vtkClientServerStream resultStream;
  int ret = func(this, tmp,method,msg,&resultStream);
  resultStream.GetData(&result, &resultLen);

  // store the last result
  if (resultLen > 0 && result)
    {
    if (this->LastResultMessage)
      {
      delete this->LastResultMessage;
      this->LastResultMessage = NULL;
      }
    const unsigned char *nextPos;
    this->LastResultMessage = vtkClientServerMessage::GetMessage(result,resultLen,
                                                        &nextPos);
    if ( this->LastResultMessage )
      {
      // store the data with this message, we need to keep it around.
      this->LastResultMessage->SetArgumentData(result, resultLen);
      this->LastResultMessage->ArgumentDataLength = resultLen;
      }
    else
      {
      cout << "Resulting message returned 0" << endl;
      }
    }
  
  delete [] method;
  delete msg;
  return ret;
}
  
char *vtkClientServerInterpreter::GetString(vtkClientServerMessage *amsg, int arg)
{
  char *method =
    new char [amsg->ArgumentSizes[arg]+1];
  memcpy(method, amsg->Arguments[arg], amsg->ArgumentSizes[arg]);
  method[amsg->ArgumentSizes[arg]] = '\0';
  return method;
}


void vtkClientServerInterpreter::AllocateMessageBuffer(vtkIdType len)
{
  if (len > this->MessageBufferSize)
    {
    delete[] this->MessageBuffer;
    this->MessageBuffer = new unsigned char[len];
    this->MessageBufferSize = len;
    }
}

