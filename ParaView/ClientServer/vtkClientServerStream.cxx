/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerStream.cxx
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
#include "vtkClientServerStream.h"

#include "vtkClientServerArrayInformation.h"
#include "vtkString.h"

vtkClientServerStream & vtkClientServerStream::operator << (vtkClientServerStream::Commands t)
{ 
  const unsigned char *uc = reinterpret_cast<const unsigned char *>(&t);
  size_t nsize = this->DataSize + sizeof(vtkClientServerStream::Commands);
  this->Data.resize(nsize);
  unsigned char* ptr = &(*this->Data.begin()) + this->DataSize;
  memcpy(ptr, uc, sizeof(vtkClientServerStream::Commands));
  this->DataSize = nsize;
  return *this;
}

vtkClientServerStream & vtkClientServerStream::operator << (vtkClientServerStream::Types t)
{ 
  const unsigned char *uc = reinterpret_cast<const unsigned char *>(&t);
  size_t nsize = this->DataSize + sizeof(vtkClientServerStream::Types);
  this->Data.resize(nsize);
  unsigned char* ptr = &(*this->Data.begin()) + this->DataSize;
  memcpy(ptr, uc, sizeof(vtkClientServerStream::Types));
  this->DataSize = nsize;
  return *this;
}

vtkClientServerStream & vtkClientServerStream::operator << (vtkClientServerID id)
{ 
  this->InternalWrite(vtkClientServerStream::id_value, 
                      reinterpret_cast<const unsigned char *>(&id.ID),
                      sizeof(unsigned long));
  return *this;  
}

void vtkClientServerStream::InternalWrite(vtkClientServerStream::Types t, 
                                 const unsigned char *d, 
                                 size_t dsize)
{ 
  // write out the type
  *this << t;

  // write the size
  size_t size = dsize;
  const unsigned char *uc = reinterpret_cast<const unsigned char *>(&size);
  size_t nsize = this->DataSize + sizeof(unsigned int) + dsize;
  this->Data.resize(nsize);
  unsigned char* ptr = &(*this->Data.begin()) + this->DataSize;
  memcpy(ptr, uc, sizeof(unsigned int));
  ptr += sizeof(unsigned int);
  memcpy(ptr, d, dsize);  
  this->DataSize = nsize;
}

vtkClientServerStream & vtkClientServerStream::operator << (int x)
{ 
  this->InternalWrite(vtkClientServerStream::int_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(int));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (short x)
{ 
  this->InternalWrite(vtkClientServerStream::short_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(short));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (unsigned int x)
{ 
  this->InternalWrite(vtkClientServerStream::unsigned_int_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(unsigned int));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (unsigned short x)
{ 
  this->InternalWrite(vtkClientServerStream::unsigned_short_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(unsigned short));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (long x)
{ 
  this->InternalWrite(vtkClientServerStream::long_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(long));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (unsigned long x)
{ 
  this->InternalWrite(vtkClientServerStream::unsigned_long_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(unsigned long));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (float x)
{ 
  this->InternalWrite(vtkClientServerStream::float_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(float));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (double x)
{ 
  this->InternalWrite(vtkClientServerStream::double_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(double));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (char x)
{ 
  this->InternalWrite(vtkClientServerStream::char_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(char));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (unsigned char x)
{ 
  this->InternalWrite(vtkClientServerStream::unsigned_char_value, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(unsigned char));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (const char *x)
{ 
  this->InternalWrite(vtkClientServerStream::string_value, 
                      reinterpret_cast<const unsigned char *>(x),
                      strlen(x));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (vtkObjectBase *x)
{ 
  this->InternalWrite(vtkClientServerStream::vtk_object_pointer, 
                      reinterpret_cast<const unsigned char *>(&x),
                      sizeof(vtkObject *));
  return *this;  
}

vtkClientServerStream & vtkClientServerStream::operator << (vtkClientServerArrayInformation a)
{ 
  this->InternalWrite(a.Type,a.Data,a.Size);
  return *this;  
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(float *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::float_array;
  a.Size = sizeof(float)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(double *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::double_array;
  a.Size = sizeof(double)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(short *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::short_array;
  a.Size = sizeof(short)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(int *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::int_array;
  a.Size = sizeof(int)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(long *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::long_array;
  a.Size = sizeof(long)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(char *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::char_array;
  a.Size = sizeof(char)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(unsigned short *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::unsigned_short_array;
  a.Size = sizeof(unsigned short)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(unsigned int *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::unsigned_int_array;
  a.Size = sizeof(unsigned int)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(unsigned long *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::unsigned_long_array;
  a.Size = sizeof(unsigned long)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

vtkClientServerArrayInformation vtkClientServerStream::InsertArray(unsigned char *f, int sz)
{
  vtkClientServerArrayInformation a;
  a.Type = vtkClientServerStream::unsigned_char_array;
  a.Size = sizeof(unsigned char)*sz;
  a.Data = reinterpret_cast<const unsigned char *>(f);
  return a;
}

void vtkClientServerStream::GetData(const unsigned char **result, size_t *resultSize)
{
  *resultSize = this->DataSize;
  *result = &(*this->Data.begin());
}

void vtkClientServerStream::Reserve(size_t len)
{
  this->Data.reserve(len);
}

void vtkClientServerStream::Reset()
{
  this->Data.erase(this->Data.begin(), this->Data.end());
  this->DataSize = 0;
}

vtkClientServerStream::vtkClientServerStream()
{
  // id == 1 is master node
  // id == 2 is error messages
  this->UniqueID.ID = 2;
  this->LastResult.ID = 0;
  this->Reserve(1024);
  this->DataSize = 0;
}

vtkClientServerStream::~vtkClientServerStream()
{
}

vtkClientServerID vtkClientServerStream::GetMasterNodeId()
{
  vtkClientServerID id;
  id.ID = 1;
  return id;
}

vtkClientServerID vtkClientServerStream::GetErrorMessageId()
{
  vtkClientServerID id;
  id.ID = 2;
  return id;
}

vtkClientServerID vtkClientServerStream::GetUniqueID()
{
  this->UniqueID.ID++;
  return this->UniqueID;
}

static const char* vtkClientServerStreamTypes[] = { 
  "float_value", 
  "float_array", 
  "double_value", 
  "double_array",
  "int_value", 
  "int_array",
  "unsigned_int_value", 
  "unsigned_int_array",
  "short_value", 
  "short_array",
  "unsigned_short_value", 
  "unsigned_short_array",
  "long_value", 
  "long_array",
  "unsigned_long_value", 
  "unsigned_long_array",
  "char_value", 
  "char_array",
  "unsigned_char_value", 
  "unsigned_char_array",
  "vtk_object_pointer",
  "string_value",
  "string_array",
  "id_value",
  "boolean_value",
  "event_value",
  "End"
};

unsigned int vtkClientServerStream::GetTypeFromString(const char* type)
{
  unsigned int cc;
  for ( cc =0; cc < vtkClientServerStream::End; cc ++ )
    {
    if ( vtkString::EqualsCase(type, vtkClientServerStreamTypes[cc]) )
      {
      return cc;
      }
    }
  return vtkClientServerStream::End;
}

const char* vtkClientServerStream::GetTypeString(unsigned int id)
{
  if ( id > vtkClientServerStream::End )
    {
    return "unknown";
    }
  return vtkClientServerStreamTypes[id];
}

