/*=========================================================================

Copyright (c) 2017, Los Alamos National Security, LLC

All rights reserved.

Copyright 2017. Los Alamos National Security, LLC.
This software was produced under U.S. Government contract DE-AC52-06NA25396
for Los Alamos National Laboratory (LANL), which is operated by
Los Alamos National Security, LLC for the U.S. Department of Energy.
The U.S. Government has rights to use, reproduce, and distribute this software.
NEITHER THE GOVERNMENT NOR LOS ALAMOS NATIONAL SECURITY, LLC MAKES ANY WARRANTY,
EXPRESS OR IMPLIED, OR ASSUMES ANY LIABILITY FOR THE USE OF THIS SOFTWARE.
If software is modified to produce derivative works, such modified software
should be clearly marked, so as not to confuse it with the version available
from LANL.

Additionally, redistribution and use in source and binary forms, with or
without modification, are permitted provided that the following conditions
are met:
-   Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
-   Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
-   Neither the name of Los Alamos National Security, LLC, Los Alamos National
    Laboratory, LANL, the U.S. Government, nor the names of its contributors
    may be used to endorse or promote products derived from this software
    without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY LOS ALAMOS NATIONAL SECURITY, LLC AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL LOS ALAMOS NATIONAL SECURITY, LLC OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/

#ifndef _GIO_PV_GIO_DATA_H_
#define _GIO_PV_GIO_DATA_H_

#include "strConvert.h"
#include <string>

namespace GIOPvPlugin
{

//
// Stores the genericIO data being read in
class GioData
{
public: // coz too lazy to do private!
  int id;
  std::string name;
  int size; // in bytes
  bool isFloat;
  bool isSigned;
  bool ghost;
  bool xVar, yVar, zVar;
  void* data;

  std::string dataType;
  bool queryOn;
  size_t numElements;

public:
  GioData();
  ~GioData();

  void init(int _id, std::string _name, int _size, bool _isFloat, bool _isSigned);
  void setNumElements(size_t _numElements) { numElements = _numElements; }

  int allocateMem(int offset = 1);
  int deAllocateMem();
  int determineDataType();

  bool greaterEqual(std::string value, std::string _dataType, size_t index);
  bool equal(std::string value, std::string _dataType, size_t index);
  bool lessEqual(std::string value, std::string _dataType, size_t index);
  bool isBetween(std::string value1, std::string value2, std::string _dataType, size_t index);

  template <typename T>
  T getValue(size_t index);

  template <typename T>
  int getValue(size_t index, T& value);

  template <typename T>
  void setValue(size_t index, T value);
};

inline GioData::GioData()
{
  dataType = "";
  numElements = 0;
  data = NULL;
  xVar = yVar = zVar = false;
  queryOn = false;
}

inline GioData::~GioData()
{
  dataType = "";
  numElements = 0;
  deAllocateMem();
}

inline void GioData::init(int _id, std::string _name, int _size, bool _isFloat, bool _isSigned)
{
  id = _id;
  name = _name;
  size = _size;
  isFloat = _isFloat;
  isSigned = _isSigned;
}

inline int GioData::determineDataType()
{
  if (isFloat)
  {
    if (size == 4)
      dataType = "float";
    else if (size == 8)
      dataType = "double";
    else
      return 0;
  }
  else // not float
  {
    if (isSigned)
    {
      if (size == 1)
        dataType = "int8_t";
      else if (size == 2)
        dataType = "int16_t";
      else if (size == 4)
        dataType = "int32_t";
      else if (size == 8)
        dataType = "int64_t";
      else
        return 0;
    }
    else
    {
      if (size == 1)
        dataType = "uint8_t";
      else if (size == 2)
        dataType = "uint16_t";
      else if (size == 4)
        dataType = "uint32_t";
      else if (size == 8)
        dataType = "uint64_t";
      else
        return 0;
    }
  }

  return 1;
}

inline int GioData::allocateMem(int offset)
{
  determineDataType();

  if (dataType == "float")
    data = new float[numElements + offset];
  else if (dataType == "double")
    data = new double[numElements + offset];
  else if (dataType == "int8_t")
    data = new int8_t[numElements + offset];
  else if (dataType == "int16_t")
    data = new int16_t[numElements + offset];
  else if (dataType == "int32_t")
    data = new int32_t[numElements + offset];
  else if (dataType == "int64_t")
    data = new int64_t[numElements + offset];
  else if (dataType == "uint8_t")
    data = new uint8_t[numElements + offset];
  else if (dataType == "uint16_t")
    data = new uint16_t[numElements + offset];
  else if (dataType == "uint32_t")
    data = new uint32_t[numElements + offset];
  else if (dataType == "uint64_t")
    data = new uint64_t[numElements + offset];
  else
    return 0;

  return 1;
}

inline int GioData::deAllocateMem()
{
  if (data == NULL) // already deallocated!
    return 1;

  if (dataType == "float")
    delete[](float*) data;
  else if (dataType == "double")
    delete[](double*) data;
  else if (dataType == "int8_t")
    delete[](int8_t*) data;
  else if (dataType == "int16_t")
    delete[](int16_t*) data;
  else if (dataType == "int32_t")
    delete[](int32_t*) data;
  else if (dataType == "int64_t")
    delete[](int64_t*) data;
  else if (dataType == "uint8_t")
    delete[](uint8_t*) data;
  else if (dataType == "uint16_t")
    delete[](uint16_t*) data;
  else if (dataType == "uint32_t")
    delete[](uint32_t*) data;
  else if (dataType == "uint64_t")
    delete[](uint64_t*) data;
  else
    return 0;

  data = NULL;

  return 1;
}

inline bool GioData::equal(std::string value, std::string _dataType, size_t index)
{
  bool matched = false;

  if (_dataType == "float")
    matched = (((float*)data)[index] == to_float(value));
  else if (_dataType == "double")
    matched = (((double*)data)[index] == to_double(value));
  else if (_dataType == "int8_t")
    matched = (((int8_t*)data)[index] == to_int8(value));
  else if (_dataType == "int16_t")
    matched = (((int16_t*)data)[index] == to_int16(value));
  else if (_dataType == "int32_t")
    matched = (((int32_t*)data)[index] == to_int32(value));
  else if (_dataType == "int64_t")
    matched = (((int64_t*)data)[index] == to_int64(value));
  else if (_dataType == "uint8_t")
    matched = (((uint8_t*)data)[index] == to_uint8(value));
  else if (_dataType == "uint16_t")
    matched = (((uint16_t*)data)[index] == to_uint16(value));
  else if (_dataType == "uint32_t")
    matched = (((uint32_t*)data)[index] == to_uint32(value));
  else if (_dataType == "uint64_t")
    matched = (((uint64_t*)data)[index] == to_uint64(value));

  return matched;
}

inline bool GioData::greaterEqual(std::string value, std::string _dataType, size_t index)
{
  bool matched = false;

  if (_dataType == "float")
    matched = (((float*)data)[index] >= to_float(value));
  else if (_dataType == "double")
    matched = (((double*)data)[index] >= to_double(value));
  else if (_dataType == "int8_t")
    matched = (((int8_t*)data)[index] >= to_int8(value));
  else if (_dataType == "int16_t")
    matched = (((int16_t*)data)[index] >= to_int16(value));
  else if (_dataType == "int32_t")
    matched = (((int32_t*)data)[index] >= to_int32(value));
  else if (_dataType == "int64_t")
    matched = (((int64_t*)data)[index] >= to_int64(value));
  else if (_dataType == "uint8_t")
    matched = (((uint8_t*)data)[index] >= to_uint8(value));
  else if (_dataType == "uint16_t")
    matched = (((uint16_t*)data)[index] >= to_uint16(value));
  else if (_dataType == "uint32_t")
    matched = (((uint32_t*)data)[index] >= to_uint32(value));
  else if (_dataType == "uint64_t")
    matched = (((uint64_t*)data)[index] >= to_uint64(value));

  return matched;
}

inline bool GioData::lessEqual(std::string value, std::string _dataType, size_t index)
{
  bool matched = false;

  if (_dataType == "float")
    matched = (((float*)data)[index] <= to_float(value));
  else if (_dataType == "double")
    matched = (((double*)data)[index] <= to_double(value));
  else if (_dataType == "int8_t")
    matched = (((int8_t*)data)[index] <= to_int8(value));
  else if (_dataType == "int16_t")
    matched = (((int16_t*)data)[index] <= to_int16(value));
  else if (_dataType == "int32_t")
    matched = (((int32_t*)data)[index] <= to_int32(value));
  else if (_dataType == "int64_t")
    matched = (((int64_t*)data)[index] <= to_int64(value));
  else if (_dataType == "uint8_t")
    matched = (((uint8_t*)data)[index] <= to_uint8(value));
  else if (_dataType == "uint16_t")
    matched = (((uint16_t*)data)[index] <= to_uint16(value));
  else if (_dataType == "uint32_t")
    matched = (((uint32_t*)data)[index] <= to_uint32(value));
  else if (_dataType == "uint64_t")
    matched = (((uint64_t*)data)[index] <= to_uint64(value));

  return matched;
}

inline bool GioData::isBetween(
  std::string value1, std::string value2, std::string _dataType, size_t index)
{
  bool matched = false;

  if (_dataType == "float")
    matched =
      (((float*)data)[index] >= to_float(value1) && ((float*)data)[index] <= to_float(value2));
  else if (_dataType == "double")
    matched =
      (((double*)data)[index] >= to_double(value1) && ((double*)data)[index] <= to_double(value2));
  else if (_dataType == "int8_t")
    matched =
      (((int8_t*)data)[index] >= to_int8(value1) && ((int8_t*)data)[index] <= to_int8(value2));
  else if (_dataType == "int16_t")
    matched =
      (((int16_t*)data)[index] >= to_int16(value1) && ((int16_t*)data)[index] <= to_int16(value2));
  else if (_dataType == "int32_t")
    matched =
      (((int32_t*)data)[index] >= to_int32(value1) && ((int32_t*)data)[index] <= to_int32(value2));
  else if (_dataType == "int64_t")
    matched =
      (((int64_t*)data)[index] >= to_int64(value1) && ((int64_t*)data)[index] <= to_int64(value2));
  else if (_dataType == "uint8_t")
    matched =
      (((uint8_t*)data)[index] >= to_uint8(value1) && ((uint8_t*)data)[index] <= to_uint8(value2));
  else if (_dataType == "uint16_t")
    matched = (((uint16_t*)data)[index] >= to_uint16(value1) &&
      ((uint16_t*)data)[index] <= to_uint16(value2));
  else if (_dataType == "uint32_t")
    matched = (((uint32_t*)data)[index] >= to_uint32(value1) &&
      ((uint32_t*)data)[index] <= to_uint32(value2));
  else if (_dataType == "uint64_t")
    matched = (((uint64_t*)data)[index] >= to_uint64(value1) &&
      ((uint64_t*)data)[index] <= to_uint64(value2));

  return matched;
}

template <typename T>
inline T GioData::getValue(size_t index)
{
  T* dataPtr = ((T*)data)[index];
  return dataPtr[index];
}

template <typename T>
inline int GioData::getValue(size_t index, T& value)
{
  if (index < numElements)
  {
    value = ((T*)data)[index];
    return 1;
  }

  return 0;
}

template <typename T>
inline void GioData::setValue(size_t index, T value)
{
  ((T*)data)[index] = value;
}

} // GIOPvPlugin namespace

#endif
