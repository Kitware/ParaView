/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerMessage.cxx
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

#include "vtkClientServerMessage.h"

vtkClientServerMessage::vtkClientServerMessage() 
{
  this->NumberOfArguments = 0;
  this->ArgumentData = 0;
}

vtkClientServerMessage::~vtkClientServerMessage() 
{
  if (this->NumberOfArguments)
    {
    delete [] this->Arguments;
    delete [] this->ArgumentTypes;
    delete [] this->ArgumentSizes;
    }
  if (this->ArgumentData)
    {
    delete [] this->ArgumentData;
    }
}

void vtkClientServerMessage::SetArgumentData(const unsigned char* data, size_t len)
{
  if ( this->ArgumentData == data )
    {
    return;
    }
  if ( this->ArgumentData )
    {
    delete [] this->ArgumentData;
    this->ArgumentData = 0;
    }
  if ( data )
    {
    this->ArgumentData = new unsigned char[len];
    memcpy(this->ArgumentData, data, len);
    // update the arguments to point to the new
    // argumentData that was just allocated.
    // It gets updated to point to the same offset as
    // it was when it pointed to data.   So, you subtract data
    // from the argument to get the offset then add the new Argumentdata
    // back in
    for(unsigned int i = 0; i < this->NumberOfArguments; ++i)
      {
      this->Arguments[i] = this->Arguments[i] - data + this->ArgumentData;
      }
    }
  
}

void vtkClientServerMessage::Copy(vtkClientServerMessage *in)
{
  size_t totalSize = 0;
  
  if (!in)
    {
    vtkGenericWarningMacro("Attempt to copy an empty message");
    return;
    }
  this->NumberOfArguments = in->NumberOfArguments;
  this->ArgumentTypes = 
    new vtkClientServerStream::Types [in->NumberOfArguments];
  this->ArgumentSizes = new unsigned int [in->NumberOfArguments];
  this->Arguments = 
    new const unsigned char * [in->NumberOfArguments];

  unsigned int i;
  // we must copy the data
  for (i = 0; i < in->NumberOfArguments; ++i)
    {
    totalSize += in->ArgumentSizes[i];
    }
  this->ArgumentData = new unsigned char [totalSize];
  unsigned char *argPtr = this->ArgumentData;
  const unsigned char *inPtr = in->Arguments[0];
  for (i = 0; i < in->NumberOfArguments; ++i)
    {
    this->ArgumentTypes[i] = in->ArgumentTypes[i];
    this->ArgumentSizes[i] = in->ArgumentSizes[i];
    memcpy(argPtr,inPtr,in->ArgumentSizes[i]);
    this->Arguments[i] = argPtr;
    argPtr += this->ArgumentSizes[i];
    inPtr += this->ArgumentSizes[i];    
    }
}


vtkClientServerMessage *vtkClientServerMessage::GetMessage(const unsigned char *msg, size_t len,
                                         const unsigned char **nextPos)
{
  if (len < (sizeof(vtkClientServerStream::Commands) + sizeof(vtkClientServerStream::Types)))
    {
    vtkGenericWarningMacro("Bad message received");
    return 0;
    }
  vtkClientServerMessage *amsg = new vtkClientServerMessage;
  amsg->Command = *reinterpret_cast<const vtkClientServerStream::Commands *>(msg);
  if (amsg->Command < vtkClientServerStream::New ||
      amsg->Command >=  vtkClientServerStream::EndOfCommands)
    {
    vtkGenericWarningMacro("Bad message received: Command " 
                           << amsg->Command << " does not exists. "
                           "Should be between: " 
                           << vtkClientServerStream::New << " and "
                           << vtkClientServerStream::EndOfCommands);
    delete amsg;
    return 0;
    }

  amsg->NumberOfArguments = 0;
  const unsigned char *msgPos = msg + sizeof(vtkClientServerStream::Commands);
  
  // find the number of arguments
  vtkClientServerStream::Types argType = 
    *reinterpret_cast<const vtkClientServerStream::Types *>(msgPos);
  msgPos += sizeof(vtkClientServerStream::Types);
  while (argType != vtkClientServerStream::End)
    {
    unsigned int as = 
      *reinterpret_cast<const unsigned int *>(msgPos);
    msgPos += sizeof(unsigned int);
    msgPos += as;
    amsg->NumberOfArguments++;
    argType = *reinterpret_cast<const vtkClientServerStream::Types *>(msgPos);
    msgPos += sizeof(vtkClientServerStream::Types);
    }
  *nextPos = msgPos;
  
  // allocate storage and fill in the structure
  if (amsg->NumberOfArguments)
    {
    amsg->ArgumentTypes = 
      new vtkClientServerStream::Types [amsg->NumberOfArguments];
    amsg->ArgumentSizes = new unsigned int [amsg->NumberOfArguments];
    amsg->Arguments = 
      new const unsigned char * [amsg->NumberOfArguments];
    
    msgPos = msg + sizeof(vtkClientServerStream::Commands);
    unsigned int i;
    for (i = 0; i < amsg->NumberOfArguments; ++i)
      {
      amsg->ArgumentTypes[i] = 
        *reinterpret_cast<const vtkClientServerStream::Types *>(msgPos);
      msgPos += sizeof(vtkClientServerStream::Types);
      amsg->ArgumentSizes[i] = 
        *reinterpret_cast<const unsigned int *>(msgPos);
      msgPos += sizeof(unsigned int);
      amsg->Arguments[i] = msgPos;
      msgPos += amsg->ArgumentSizes[i];
      }
    }
  return amsg;
}
