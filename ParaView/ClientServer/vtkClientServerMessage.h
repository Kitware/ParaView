/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerMessage.h
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

#ifndef __vtkClientServerMessage_h
#define __vtkClientServerMessage_h

#include "vtkClientServerStream.h"


class VTK_EXPORT vtkClientServerMessage
{
public:
  vtkClientServerStream::Commands Command;
  unsigned int NumberOfArguments;
  // argument sizes are in bytes
  unsigned int *ArgumentSizes;
  vtkClientServerStream::Types *ArgumentTypes;
  const unsigned char **Arguments;
  unsigned char *ArgumentData;
  size_t ArgumentDataLength;
  
  // Description:
  // copy a Message to another one
  void Copy(vtkClientServerMessage *);
  
  void SetArgumentData(const unsigned char* data, size_t len);

  // Description:
  // Get message out of stream.
  static vtkClientServerMessage *GetMessage(const unsigned char *msg, size_t len,
                                   const unsigned char **nextPos);
  vtkClientServerMessage();
  ~vtkClientServerMessage();
};

#endif
