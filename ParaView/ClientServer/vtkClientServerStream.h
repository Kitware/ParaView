/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerStream.h
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
#ifndef __vtkClientServerStream_h
#define __vtkClientServerStream_h

#include "vtkObject.h"

#include <vector>

struct vtkClientServerArrayInformation;

struct vtkClientServerID
{
  int operator<(const vtkClientServerID& i) const
    {
      return this->ID < i.ID;
    }
  int operator==(const vtkClientServerID& i) const
    {
      return this->ID == i.ID;
    }
  unsigned long ID;
};


class VTK_EXPORT vtkClientServerStream
{
public:
  enum Commands { New, Invoke, Delete, AssignResult, Reply, EndOfCommands};
  enum Types { float_value, float_array, 
               double_value, double_array,
               int_value, int_array,
               unsigned_int_value, unsigned_int_array,
               short_value, short_array,
               unsigned_short_value, unsigned_short_array,
               long_value, long_array,
               unsigned_long_value, unsigned_long_array,
               char_value, char_array,
               unsigned_char_value, unsigned_char_array,
               vtk_object_pointer, string_value, string_array, 
               id_value, 
               boolean_value, event_value, End
  };
  static const char* GetTypeString(unsigned int);
  static unsigned int GetTypeFromString(const char*);
  
  static int GetCommandSize() { return sizeof(Commands); }
  // stream operators for special types
  vtkClientServerStream & operator << (vtkClientServerStream::Types t);
  vtkClientServerStream & operator << (vtkClientServerStream::Commands t);
  vtkClientServerStream & operator << (vtkClientServerArrayInformation);
  vtkClientServerStream & operator << (vtkClientServerID i);
  vtkClientServerStream & operator << (vtkObjectBase *obj);

  // stream operators for native types
  vtkClientServerStream & operator << (short x) ; // integers
  vtkClientServerStream & operator << (unsigned short x) ; // integers
  vtkClientServerStream & operator << (int x) ; // integers
  vtkClientServerStream & operator << (unsigned int x) ; // integers
  vtkClientServerStream & operator << (long l) ; // long integers
  vtkClientServerStream & operator << (unsigned long l) ; // long integers
  vtkClientServerStream & operator << (char ch) ;  // characters
  vtkClientServerStream & operator << (unsigned char ch) ;  // characters
  vtkClientServerStream & operator << (float fl ) ;  // floats
  vtkClientServerStream & operator << (double dbl);  // double floats
  vtkClientServerStream & operator << (const char *str);   // strings

  
  // allow arrays to be passed
  static vtkClientServerArrayInformation InsertArray(float *, int );
  static vtkClientServerArrayInformation InsertArray(double *, int );
  static vtkClientServerArrayInformation InsertArray(short *, int );
  static vtkClientServerArrayInformation InsertArray(int *, int );
  static vtkClientServerArrayInformation InsertArray(long *, int );
  static vtkClientServerArrayInformation InsertArray(char *, int );
  static vtkClientServerArrayInformation InsertArray(unsigned short *, int );
  static vtkClientServerArrayInformation InsertArray(unsigned int *, int );
  static vtkClientServerArrayInformation InsertArray(unsigned long *, int );
  static vtkClientServerArrayInformation InsertArray(unsigned char *, int );
    
  // return a unique ID
  vtkClientServerID GetUniqueID();
  
  vtkClientServerStream();
  ~vtkClientServerStream();

  // reset the stream to an empty state
  void Reset();
  
  // get the data as an array and size
  void GetData(const unsigned char **res, size_t *resLength);

  // Description:
  // Get master node ID.
  vtkClientServerID GetMasterNodeId();

  // Description:
  // Get error msg ID.
  vtkClientServerID GetErrorMessageId();

  // use the last result as an argument
  vtkClientServerID GetLastResult() { return this->LastResult; }
  
  // ask the container to reserve some space to avoid too many
  // memory allocations
  void Reserve(size_t len);

protected:  
  void InternalWrite(vtkClientServerStream::Types t, const unsigned char *ptr, 
                     size_t size);
  vtkstd::vector<unsigned char> Data;
  size_t DataSize;

  vtkClientServerID UniqueID;
  vtkClientServerID LastResult;
};

#endif
