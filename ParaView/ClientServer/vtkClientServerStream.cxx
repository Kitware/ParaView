/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkClientServerStream.cxx
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
#include "vtkClientServerStream.h"

#include "vtkByteSwap.h"
#include "vtkSmartPointer.h"
#include "vtkTypeFromNative.h"

#include <vtkstd/vector>

//----------------------------------------------------------------------------
// Portability of typename keyword.
#if defined(__sgi) && !defined(_COMPILER_VERSION) && !defined(__GNUC__)
# define VTK_CSS_TYPENAME
#elif defined(_MSC_VER) && (_MSC_VER < 1310)
# define VTK_CSS_TYPENAME
#else
# define VTK_CSS_TYPENAME typename
#endif

//----------------------------------------------------------------------------
// Macro to dispatch templated functions based on type in a switch.
#define VTK_CSS_TEMPLATE_MACRO(kind, call)                     \
  case int8_##kind: { vtkTypeInt8* T = 0; call; } break;       \
  case int16_##kind: { vtkTypeInt16* T = 0; call; } break;     \
  case int32_##kind: { vtkTypeInt32* T = 0; call; } break;     \
  case int64_##kind: { vtkTypeInt64* T = 0; call; } break;     \
  case uint8_##kind: { vtkTypeUInt8* T = 0; call; } break;     \
  case uint16_##kind: { vtkTypeUInt16* T = 0; call; } break;   \
  case uint32_##kind: { vtkTypeUInt32* T = 0; call; } break;   \
  case uint64_##kind: { vtkTypeUInt64* T = 0; call; } break;   \
  case float32_##kind: { vtkTypeFloat32* T = 0; call; } break; \
  case float64_##kind: { vtkTypeFloat64* T = 0; call; } break

//----------------------------------------------------------------------------
// Visual Studio 6 does not provide output operators for __int64.  We need
// to wrap up streaming output for vtkTypeInt64 and vtkTypeUInt64.
struct vtkClientServerStreamInt64
{
  vtkTypeInt64 Value;
  vtkClientServerStreamInt64(vtkTypeInt64 v): Value(v) {}
};
struct vtkClientServerStreamUInt64
{
  vtkTypeUInt64 Value;
  vtkClientServerStreamUInt64(vtkTypeUInt64 v): Value(v) {}
};
inline ostream& operator << (ostream& os, vtkClientServerStreamInt64 id)
{
#if defined(VTK_TYPE_INT64_NOT_STANDARD) && defined(_MSC_VER) && (_MSC_VER < 1300)
  // _i64toa can use up to 33 bytes (32 + null terminator).
  char buf[33];
  // Convert to string representation in base 10.
  return os << _i64toa(id.Value, buf, 10);
#else
  return os << id.Value;
#endif
}
inline ostream& operator << (ostream& os, vtkClientServerStreamUInt64 id)
{
#if defined(VTK_TYPE_INT64_NOT_STANDARD) && defined(_MSC_VER) && (_MSC_VER < 1300)
  // _i64toa can use up to 33 bytes (32 + null terminator).
  char buf[33];
  // Convert to string representation in base 10.
  return os << _ui64toa(id.Value, buf, 10);
#else
  return os << id.Value;
#endif
}

//----------------------------------------------------------------------------
// Define some traits for vtkType types.
template <class T> struct vtkClientServerTypeTraits;
#define VTK_CLIENT_SERVER_TYPE_TRAIT(in, out, print)    \
  VTK_TEMPLATE_SPECIALIZATION                           \
  struct vtkClientServerTypeTraits< in >                \
  {                                                     \
    /* Type to which to convert for display. */         \
    typedef print PrintType;                            \
    /* Type identifier for value of this type. */       \
    static vtkClientServerStream::Types Value()         \
      { return vtkClientServerStream::out##_value; }    \
    /* Type identifier for array of this type. */       \
    static vtkClientServerStream::Types Array()         \
      { return vtkClientServerStream::out##_array; }    \
  }
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt8, int8, vtkTypeInt16);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt16, int16, vtkTypeInt16);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt32, int32, vtkTypeInt32);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeInt64, int64, vtkClientServerStreamInt64);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt8, uint8, vtkTypeUInt16);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt16, uint16, vtkTypeUInt16);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt32, uint32, vtkTypeUInt32);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeUInt64, uint64, vtkClientServerStreamUInt64);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeFloat32, float32, vtkTypeFloat32);
VTK_CLIENT_SERVER_TYPE_TRAIT(vtkTypeFloat64, float64, vtkTypeFloat64);
#undef VTK_CLIENT_SERVER_TYPE_TRAIT

//----------------------------------------------------------------------------
// Internal implementation data.
class vtkClientServerStreamInternals
{
public:
  // Actual binary data in the stream.
  typedef vtkstd::vector<unsigned char> DataType;
  DataType Data;

  // Offset to each value stored in the stream.
  typedef vtkstd::vector<DataType::difference_type> ValueOffsetsType;
  ValueOffsetsType ValueOffsets;

  // Index into ValueOffsets of the first value corresponding to each
  // message.
  typedef vtkstd::vector<ValueOffsetsType::size_type> MessageIndexesType;
  MessageIndexesType MessageIndexes;

  // Hold references to vtkObjectBase instances stored in the stream.
  typedef vtkstd::vector< vtkSmartPointer<vtkObjectBase> > ObjectsType;
  ObjectsType Objects;

  // Index into ValueOffsets where the last Command started.  Used to
  // detect valid message completion.
  enum { InvalidStartIndex = 0xFFFFFFFF };
  ValueOffsetsType::size_type StartIndex;

  // Whether the stream has been constructed improperly.
  int Invalid;

  // Access to protected members of vtkClientServerStream.
  static vtkClientServerStream& Write(vtkClientServerStream& css,
                                      const void* data, size_t length)
    { return css.Write(data, length); }
  static const unsigned char* GetValue(const vtkClientServerStream& css,
                                       int message, int value)
    { return css.GetValue(message, value); }
};

//----------------------------------------------------------------------------
vtkClientServerStream::vtkClientServerStream()
{
  // Initialize the internal representation of the stream.
  this->Internal = new vtkClientServerStreamInternals;
  this->Reserve(1024);
  this->Reset();
}

//----------------------------------------------------------------------------
vtkClientServerStream::~vtkClientServerStream()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
vtkClientServerStream::vtkClientServerStream(const vtkClientServerStream& r)
{
  // Allocate and copy the internal representation of the stream.
  this->Internal = new vtkClientServerStreamInternals(*r.Internal);
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator=(const vtkClientServerStream& that)
{
  *this->Internal = *that.Internal;
  return *this;
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Copy(const vtkClientServerStream* source)
{
  *this = *source;
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::Write(const void* data, size_t length)
{
  // Make sure we have data.
  if(length == 0)
    {
    return *this;
    }
  else if(!data)
    {
    vtkGenericWarningMacro(
      "vtkClientServerStream::Write given NULL pointer and non-zero length.");
    return *this;
    }

  // Copy the value into the data.
  this->Internal->Data.resize(this->Internal->Data.size() + length);
  memcpy(&*this->Internal->Data.end() - length, data, length);
  return *this;
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Reserve(size_t size)
{
  this->Internal->Data.reserve(size);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Reset()
{
  // Empty the entire stream.
  this->Internal->Data.erase(this->Internal->Data.begin(),
                             this->Internal->Data.end());
  this->Internal->ValueOffsets.erase(this->Internal->ValueOffsets.begin(),
                                     this->Internal->ValueOffsets.end());
  this->Internal->MessageIndexes.erase(this->Internal->MessageIndexes.begin(),
                                       this->Internal->MessageIndexes.end());
  this->Internal->Objects.erase(this->Internal->Objects.begin(),
                                this->Internal->Objects.end());

  // No message has yet been started.
  this->Internal->Invalid = 0;
  this->Internal->StartIndex =
    vtkClientServerStreamInternals::InvalidStartIndex;

  // Store the byte order of data to come.
#ifdef VTK_WORDS_BIGENDIAN
  this->Internal->Data.push_back(vtkClientServerStream::BigEndian);
#else
  this->Internal->Data.push_back(vtkClientServerStream::LittleEndian);
#endif
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (vtkClientServerStream::Commands t)
{
  // Make sure we do not start another command without ending the
  // previous one.
  if(this->Internal->StartIndex !=
     vtkClientServerStreamInternals::InvalidStartIndex)
    {
    // Got two Commands without an End between them.  This is an
    // invalid stream.
    this->Internal->Invalid = 1;
    return *this;
    }

  // Save where this message starts.
  this->Internal->StartIndex = this->Internal->ValueOffsets.size();

  // The command counts as the first value in the message.
  this->Internal->ValueOffsets.push_back(
    this->Internal->Data.end()-this->Internal->Data.begin());

  // Store the command in the stream.
  vtkTypeUInt32 data = static_cast<vtkTypeUInt32>(t);
  return this->Write(&data, sizeof(data));
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (vtkClientServerStream::Types t)
{
  // If this is the end of a message, record the message start
  // position.
  if(t == vtkClientServerStream::End)
    {
    // Make sure the command we are ending actually started.
    if(this->Internal->StartIndex ==
       vtkClientServerStreamInternals::InvalidStartIndex)
      {
      // This is an invalid stream.
      this->Internal->Invalid = 1;
      return *this;
      }

    // Store the value index where this command started.
    this->Internal->MessageIndexes.push_back(this->Internal->StartIndex);

    // No current Command is being constructed.
    this->Internal->StartIndex =
      vtkClientServerStreamInternals::InvalidStartIndex;
    }

  // All values write their type first.  Mark the start of this type
  // and optional value.
  this->Internal->ValueOffsets.push_back(
    this->Internal->Data.end()-this->Internal->Data.begin());

  // Store the type in the stream.
  vtkTypeUInt32 data = static_cast<vtkTypeUInt32>(t);
  return this->Write(&data, sizeof(data));
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (vtkClientServerStream::Argument a)
{
  if(a.Data && a.Size)
    {
    // Mark the start of this type and optional value.
    this->Internal->ValueOffsets.push_back(
      this->Internal->Data.end()-this->Internal->Data.begin());

    // If the argument is a vtk_object_pointer, we need to store a
    // reference to the object.
    vtkTypeUInt32 tp;
    memcpy(&tp, a.Data, sizeof(tp));
    if(tp == vtkClientServerStream::vtk_object_pointer)
      {
      vtkObjectBase* obj;
      memcpy(&obj, a.Data+sizeof(tp), sizeof(obj));
      this->Internal->Objects.push_back(obj);
      }

    // Write the data to the stream.
    return this->Write(a.Data, a.Size);
    }
  return *this;
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (vtkClientServerStream::Array a)
{
  // Store the array type, then length, then data.
  *this << a.Type;
  this->Write(&a.Length, sizeof(a.Length));
  return this->Write(a.Data, a.Size);
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (const vtkClientServerStream& css)
{
  const unsigned char* data;
  size_t length;
  // Do not allow object pointers to be passed in binary form.
  if(this != &css && css.Internal->Objects.empty() &&
     css.GetData(&data, &length))
    {
    // Store the stream_value type, then length, then data.
    *this << vtkClientServerStream::stream_value;
    vtkTypeUInt32 size = static_cast<vtkTypeUInt32>(length);
    this->Write(&size, sizeof(size));
    return this->Write(data, size);
    }
  else
    {
    // This is an invalid stream.
    this->Internal->Invalid = 1;
    return *this;
    }
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (vtkClientServerID id)
{
  // Store the id_value type and then the id value itself.
  *this << vtkClientServerStream::id_value;
  return this->Write(&id.ID, sizeof(id.ID));
}

//----------------------------------------------------------------------------
vtkClientServerStream&
vtkClientServerStream::operator << (vtkObjectBase* obj)
{
  // The stream will now hold a reference to the object.
  this->Internal->Objects.push_back(obj);

  // Store the vtk_object_pointer type and then the pointer value itself.
  *this << vtkClientServerStream::vtk_object_pointer;
  return this->Write(&obj, sizeof(obj));
}

//----------------------------------------------------------------------------
// Template and macro to implement all insertion operators in the same way.
template <class T>
vtkClientServerStream&
vtkClientServerStreamOperatorSL(vtkClientServerStream* self, T x)
{
  // Store the type first, then the value.
  typedef VTK_CSS_TYPENAME vtkTypeFromNative<T>::Type Type;
  *self << vtkClientServerTypeTraits<Type>::Value();
  return vtkClientServerStreamInternals::Write(*self, &x, sizeof(x));
}

#define VTK_CLIENT_SERVER_OPERATOR(type)                             \
  vtkClientServerStream& vtkClientServerStream::operator << (type x) \
    {                                                                \
    return vtkClientServerStreamOperatorSL(this, x);                 \
    }
VTK_CLIENT_SERVER_OPERATOR(char)
VTK_CLIENT_SERVER_OPERATOR(int)
VTK_CLIENT_SERVER_OPERATOR(short)
VTK_CLIENT_SERVER_OPERATOR(long)
VTK_CLIENT_SERVER_OPERATOR(signed char)
VTK_CLIENT_SERVER_OPERATOR(unsigned char)
VTK_CLIENT_SERVER_OPERATOR(unsigned int)
VTK_CLIENT_SERVER_OPERATOR(unsigned short)
VTK_CLIENT_SERVER_OPERATOR(unsigned long)
#ifdef VTK_TYPE_INT64_NOT_STANDARD
VTK_CLIENT_SERVER_OPERATOR(vtkTypeInt64)
VTK_CLIENT_SERVER_OPERATOR(vtkTypeUInt64)
#endif
VTK_CLIENT_SERVER_OPERATOR(float)
VTK_CLIENT_SERVER_OPERATOR(double)
#undef VTK_CLIENT_SERVER_OPERATOR

//----------------------------------------------------------------------------
vtkClientServerStream& vtkClientServerStream::operator << (const char* x)
{
  // String length will include null terminator.  NULL string will be
  // length 0.
  vtkTypeUInt32 length = static_cast<vtkTypeUInt32>(x?(strlen(x)+1):0);
  *this << vtkClientServerStream::string_value;
  this->Write(&length, sizeof(length));
  return this->Write(x, static_cast<size_t>(length));
}

//----------------------------------------------------------------------------
// Template and macro to implement all InsertArray methods in the same way.
template <class T>
vtkClientServerStream::Array
vtkClientServerStreamInsertArray(const T* data, int length)
{
  // Construct and return the array information structure.
  typedef VTK_CSS_TYPENAME vtkTypeFromNative<T>::Type Type;
  vtkClientServerStream::Array a =
    {
      vtkClientServerTypeTraits<Type>::Array(),
      static_cast<vtkTypeUInt32>(length),
      static_cast<vtkTypeUInt32>(sizeof(Type)*length),
      data
    };
  return a;
}

#define VTK_CLIENT_SERVER_INSERT_ARRAY(type)                       \
  vtkClientServerStream::Array                                     \
  vtkClientServerStream::InsertArray(const type* data, int length) \
    {                                                              \
    return vtkClientServerStreamInsertArray(data, length);         \
    }
VTK_CLIENT_SERVER_INSERT_ARRAY(char)
VTK_CLIENT_SERVER_INSERT_ARRAY(short)
VTK_CLIENT_SERVER_INSERT_ARRAY(int)
VTK_CLIENT_SERVER_INSERT_ARRAY(long)
VTK_CLIENT_SERVER_INSERT_ARRAY(signed char)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned char)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned short)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned int)
VTK_CLIENT_SERVER_INSERT_ARRAY(unsigned long)
#ifdef VTK_TYPE_INT64_NOT_STANDARD
VTK_CLIENT_SERVER_INSERT_ARRAY(vtkTypeInt64)
VTK_CLIENT_SERVER_INSERT_ARRAY(vtkTypeUInt64)
#endif
VTK_CLIENT_SERVER_INSERT_ARRAY(float)
VTK_CLIENT_SERVER_INSERT_ARRAY(double)
#undef VTK_CLIENT_SERVER_INSERT_ARRAY

//----------------------------------------------------------------------------
// Template to implement each type conversion in the lookup tables below.
template <class SourceType, class T>
void vtkClientServerStreamGetArgumentCase(SourceType*,
                                          const unsigned char* src, T* dest)
{
  // Copy the value out of the stream and convert it.
  SourceType value;
  memcpy(&value, src, sizeof(value));
  *dest = static_cast<T>(value);
}

#define VTK_CSS_GET_ARGUMENT_CASE(TypeId, SourceType) \
  case vtkClientServerStream::TypeId:                 \
  {                                                   \
  SourceType* T = 0;                                  \
  vtkClientServerStreamGetArgumentCase(T, src, dest); \
  } break

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeInt8* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt8.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeInt16* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt16.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeInt32* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt32.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeInt64* dest)
{
  // Lookup what types can be converted safely to a vtkTypeInt64.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeUInt8* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt8.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeUInt16* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt16.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeUInt32* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt32.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeUInt64* dest)
{
  // Lookup what types can be converted safely to a vtkTypeUInt64.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint64_value, vtkTypeUInt64);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(int64_value, vtkTypeInt64);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeFloat32* dest)
{
  // Lookup what types can be converted safely to a vtkTypeFloat32.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);

    // Unsafe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default: return 0;
    };
  return 1;
}

int vtkClientServerStreamGetArgument(vtkClientServerStream::Types type,
                                     const unsigned char* src,
                                     vtkTypeFloat64* dest)
{
  // Lookup what types can be converted safely to a vtkTypeFloat64.
  switch(type)
    {
    // Safe conversions:
    VTK_CSS_GET_ARGUMENT_CASE(int8_value, vtkTypeInt8);
    VTK_CSS_GET_ARGUMENT_CASE(int16_value, vtkTypeInt16);
    VTK_CSS_GET_ARGUMENT_CASE(int32_value, vtkTypeInt32);
    VTK_CSS_GET_ARGUMENT_CASE(uint8_value, vtkTypeUInt8);
    VTK_CSS_GET_ARGUMENT_CASE(uint16_value, vtkTypeUInt16);
    VTK_CSS_GET_ARGUMENT_CASE(uint32_value, vtkTypeUInt32);
    VTK_CSS_GET_ARGUMENT_CASE(float32_value, vtkTypeFloat32);
    VTK_CSS_GET_ARGUMENT_CASE(float64_value, vtkTypeFloat64);
    default: return 0;
    };
  return 1;
}

#undef VTK_CSS_GET_ARGUMENT_CASE

//----------------------------------------------------------------------------
// Template and macro to implement value GetArgument methods in the same way.
template <class T>
int
vtkClientServerStreamGetArgumentValue(const vtkClientServerStream* self,
                                      int midx, int argument, T* value)
{
  typedef VTK_CSS_TYPENAME vtkTypeFromNative<T>::Type Type;
  if(const unsigned char* data =
     vtkClientServerStreamInternals::GetValue(*self, midx, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Call the type conversion function for this type.
    return vtkClientServerStreamGetArgument(
      static_cast<vtkClientServerStream::Types>(tp), data,
      reinterpret_cast<Type*>(value));
    }
  return 0;
}

#define VTK_CSS_GET_ARGUMENT(type)                                          \
  int vtkClientServerStream::GetArgument(int message, int argument,         \
                                         type* value) const                 \
  {                                                                         \
    return vtkClientServerStreamGetArgumentValue(this, message,             \
                                                 argument, value);          \
  }
VTK_CSS_GET_ARGUMENT(signed char)
VTK_CSS_GET_ARGUMENT(char)
VTK_CSS_GET_ARGUMENT(int)
VTK_CSS_GET_ARGUMENT(short)
VTK_CSS_GET_ARGUMENT(long)
VTK_CSS_GET_ARGUMENT(unsigned char)
VTK_CSS_GET_ARGUMENT(unsigned int)
VTK_CSS_GET_ARGUMENT(unsigned short)
VTK_CSS_GET_ARGUMENT(unsigned long)
VTK_CSS_GET_ARGUMENT(float)
VTK_CSS_GET_ARGUMENT(double)
#ifdef VTK_TYPE_INT64_NOT_STANDARD
VTK_CSS_GET_ARGUMENT(vtkTypeInt64)
VTK_CSS_GET_ARGUMENT(vtkTypeUInt64)
#endif
#undef VTK_CSS_GET_ARGUMENT

//----------------------------------------------------------------------------
// Template and macro to implement array GetArgument methods in the same way.
template <class T>
int
vtkClientServerStreamGetArgumentArray(const vtkClientServerStream* self,
                                      int midx, int argument, T* value,
                                      vtkTypeUInt32 length)
{
  typedef VTK_CSS_TYPENAME vtkTypeFromNative<T>::Type Type;
  if(const unsigned char* data =
     vtkClientServerStreamInternals::GetValue(*self, midx, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // If the type and length of the array match, use it.
    if(static_cast<vtkClientServerStream::Types>(tp) ==
       vtkClientServerTypeTraits<Type>::Array())
      {
      // Get the length of the value in the stream.
      vtkTypeUInt32 len;
      memcpy(&len, data, sizeof(len));
      data += sizeof(len);

      if(len == length)
        {
        // Copy the value out of the stream.
        memcpy(value, data, len*sizeof(Type));
        return 1;
        }
      }
    }
  return 0;
}

#define VTK_CSS_GET_ARGUMENT_ARRAY(type)                                  \
  int vtkClientServerStream::GetArgument(int message, int argument,       \
                                         type* value,                     \
                                         vtkTypeUInt32 length) const      \
  {                                                                       \
    return vtkClientServerStreamGetArgumentArray(this, message, argument, \
                                                 value, length);          \
  }
VTK_CSS_GET_ARGUMENT_ARRAY(signed char)
VTK_CSS_GET_ARGUMENT_ARRAY(char)
VTK_CSS_GET_ARGUMENT_ARRAY(int)
VTK_CSS_GET_ARGUMENT_ARRAY(short)
VTK_CSS_GET_ARGUMENT_ARRAY(long)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned char)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned int)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned short)
VTK_CSS_GET_ARGUMENT_ARRAY(unsigned long)
VTK_CSS_GET_ARGUMENT_ARRAY(float)
VTK_CSS_GET_ARGUMENT_ARRAY(double)
#ifdef VTK_TYPE_INT64_NOT_STANDARD
VTK_CSS_GET_ARGUMENT_ARRAY(vtkTypeInt64)
VTK_CSS_GET_ARGUMENT_ARRAY(vtkTypeUInt64)
#endif
#undef VTK_CSS_GET_ARGUMENT_ARRAY

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument,
                                       const char** value) const
{
  // Get a pointer to the type/value pair in the stream.
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is a string.
    if(static_cast<vtkClientServerStream::Types>(tp) ==
       vtkClientServerStream::string_value)
      {
      // Get the length of the string.
      vtkTypeUInt32 len;
      memcpy(&len, data, sizeof(len));
      data += sizeof(len);

      if(len > 0)
        {
        // Give back a pointer directly to the string in the stream.
        *value = reinterpret_cast<const char*>(data);
        }
      else
        {
        // String pointer value was NULL.
        *value = 0;
        }
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument,
                                       char** value) const
{
  const char* tmp;
  if(this->GetArgument(message, argument, &tmp))
    {
    *value = const_cast<char*>(tmp);
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument,
                                       vtkClientServerStream* value) const
{
  // Get a pointer to the type/value pair in the stream.
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is a stream_value.
    if(static_cast<vtkClientServerStream::Types>(tp) ==
       vtkClientServerStream::stream_value)
      {
      // Get the length of the stream data.
      vtkTypeUInt32 len;
      memcpy(&len, data, sizeof(len));
      data += sizeof(len);

      // Set the data in the given stream.
      size_t length = static_cast<size_t>(len);
      return value->SetData(data, length);
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgument(int message, int argument,
                                       vtkClientServerID* value) const
{
  // Get a pointer to the type/value pair in the stream.
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is an id_value.
    if(static_cast<vtkClientServerStream::Types>(tp) ==
       vtkClientServerStream::id_value)
      {
      // Copy the value out of the stream.
      memcpy(&value->ID, data, sizeof(value->ID));
      return 1;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
// Template to implement GetArgument zero-to-null-pointer conversions.
template <class SourceType>
int vtkClientServerStreamGetArgumentPointer(SourceType*,
                                            const unsigned char* src,
                                            vtkObjectBase** dest)
{
  // Copy the value out of the stream.
  SourceType value;
  memcpy(&value, src, sizeof(value));

  // Values of 0 can be converted to a NULL pointer.
  if(value == static_cast<SourceType>(0))
    {
    *dest = 0;
    return 1;
    }
  return 0;
}

int vtkClientServerStream::GetArgument(int message, int argument,
                                       vtkObjectBase** value) const
{
  // Get a pointer to the type/value pair in the stream.
  int result = 0;
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is a vtk_object_pointer or is 0.
    switch(static_cast<vtkClientServerStream::Types>(tp))
      {
      VTK_CSS_TEMPLATE_MACRO(value, result =
                             vtkClientServerStreamGetArgumentPointer
                             (T, data, value));
      case vtkClientServerStream::vtk_object_pointer:
        {
        // Copy the value out of the stream.
        memcpy(value, data, sizeof(*value));
        result = 1;
        } break;
      case vtkClientServerStream::id_value:
        {
        // Copy the value out of the stream.
        vtkClientServerID id;
        memcpy(&id.ID, data, sizeof(id.ID));
        // ID value 0 can be converted to a NULL pointer.
        if(id.ID == 0)
          {
          *value = 0;
          return 1;
          }
        return 0;
        } break;
      default:
        break;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgumentLength(int message, int argument,
                                             vtkTypeUInt32* length) const
{
  // Get a pointer to the type/value pair in the stream.
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Make sure the type is an array type.
    switch(static_cast<vtkClientServerStream::Types>(tp))
      {
      case vtkClientServerStream::int8_array:
      case vtkClientServerStream::uint8_array:
      case vtkClientServerStream::int16_array:
      case vtkClientServerStream::uint16_array:
      case vtkClientServerStream::int32_array:
      case vtkClientServerStream::uint32_array:
      case vtkClientServerStream::float32_array:
      case vtkClientServerStream::int64_array:
      case vtkClientServerStream::uint64_array:
      case vtkClientServerStream::float64_array:
        {
        // Get the length of the array.
        memcpy(length, data, sizeof(*length));
        } return 1;
      default: break;
      }
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetArgumentObject(int message, int argument,
                                             vtkObjectBase** value,
                                             const char* type) const
{
  // Get the argument object and check its type.
  vtkObjectBase* obj;
  if(this->GetArgument(message, argument, &obj))
    {
    // if the pointer is valid then check the type
    if(obj && !obj->IsA(type))
      {
      return 0;
      }
    *value = obj;
    return 1;
    }
  return 0;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetData(const unsigned char** data,
                                   size_t* length) const
{
  // Do not return data unless stream is valid.
  if(!this->Internal->Invalid)
    {
    if(data)
      {
      *data = &*this->Internal->Data.begin();
      }

    if(length)
      {
      *length = this->Internal->Data.size();
      }
    return 1;
    }
  else
    {
    if(data)
      {
      *data = 0;
      }

    if(length)
      {
      *length = 0;
      }
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::SetData(const unsigned char* data, size_t length)
{
  // Reset and remove the byte order entry from the stream.
  this->Reset();
  this->Internal->Data.erase(this->Internal->Data.begin(),
                             this->Internal->Data.end());

  // Store the given data in the stream.
  if(data)
    {
    this->Internal->Data.insert(this->Internal->Data.begin(), data, data+length);
    }

  // Parse the stream to fill in ValueOffsets and MessageIndexes and
  // to perform byte-swapping if necessary.
  if(this->ParseData())
    {
    // Data have been byte-swapped to the native representation.
#ifdef VTK_WORDS_BIGENDIAN
    this->Internal->Data[0] = vtkClientServerStream::BigEndian;
#else
    this->Internal->Data[0] = vtkClientServerStream::LittleEndian;
#endif
    return 1;
    }
  else
    {
    // Data are invalid.  Reset the stream and report failure.
    this->Reset();
    return 0;
    }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::ParseData()
{
  // We are not modifying the vector size.  It is safe to use pointers
  // into it.
  unsigned char* begin = &*this->Internal->Data.begin();
  unsigned char* end = &*this->Internal->Data.end();

  // Make sure we have at least one byte.
  if(begin == end)
    {
    return 0;
    }

  // Save the byte order.
  int order = *begin;

  // Traverse the data until no more commands are found.
  unsigned char* data = begin+1;
  while(data && data < end)
    {
    // Parse the command.
    data = this->ParseCommand(order, data, begin, end);

    // Parse the arguments until End is reached.
    int foundEnd = 0;
    while(!foundEnd && data && data < end)
      {
      // Parse the type identifier.
      vtkClientServerStream::Types type;
      data = this->ParseType(order, data, begin, end, &type);
      if(!data)
        {
        break;
        }

      // Process the rest of the value based on its type.
      switch(type)
        {
        case vtkClientServerStream::int8_value:
        case vtkClientServerStream::uint8_value:
          data = this->ParseValue(order, data, end, 1); break;
        case vtkClientServerStream::int8_array:
        case vtkClientServerStream::uint8_array:
          data = this->ParseArray(order, data, end, 1); break;
        case vtkClientServerStream::int16_value:
        case vtkClientServerStream::uint16_value:
          data = this->ParseValue(order, data, end, 2); break;
        case vtkClientServerStream::int16_array:
        case vtkClientServerStream::uint16_array:
          data = this->ParseArray(order, data, end, 2); break;
        case vtkClientServerStream::id_value:
        case vtkClientServerStream::int32_value:
        case vtkClientServerStream::uint32_value:
        case vtkClientServerStream::float32_value:
          data = this->ParseValue(order, data, end, 4); break;
        case vtkClientServerStream::int32_array:
        case vtkClientServerStream::uint32_array:
        case vtkClientServerStream::float32_array:
          data = this->ParseArray(order, data, end, 4); break;
        case vtkClientServerStream::int64_value:
        case vtkClientServerStream::uint64_value:
        case vtkClientServerStream::float64_value:
          data = this->ParseValue(order, data, end, 8); break;
        case vtkClientServerStream::int64_array:
        case vtkClientServerStream::uint64_array:
        case vtkClientServerStream::float64_array:
          data = this->ParseArray(order, data, end, 8); break;
        case vtkClientServerStream::string_value:
          data = this->ParseString(order, data, end); break;
        case vtkClientServerStream::stream_value:
          data = this->ParseStream(order, data, end); break;
        case vtkClientServerStream::LastResult:
          // There are no data for this type.  Do nothing.
          break;
        case vtkClientServerStream::End:
          this->ParseEnd(); foundEnd = 1; break;

        // An object pointer cannot be safely transferred in a buffer.
        case vtkClientServerStream::vtk_object_pointer:
        default: /* ERROR */
          data = 0;
          break;
        }
      }
    }

  // Return whether parsing finished.
  return (data == end)? 1:0;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseCommand(int order,
                                                   unsigned char* data,
                                                   unsigned char* begin,
                                                   unsigned char* end)
{
  // Byte-swap this command's identifier.
  if(data > end-sizeof(vtkTypeUInt32))
    {
    /* ERROR */
    return 0;
    }
  this->PerformByteSwap(order, data, 1, sizeof(vtkTypeUInt32));

  // Mark the start of the command.
  this->Internal->StartIndex =
    this->Internal->ValueOffsets.end()-this->Internal->ValueOffsets.begin();
  this->Internal->ValueOffsets.push_back(data - begin);

  // Return the position after the command identifier.
  return data + sizeof(vtkTypeUInt32);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::ParseEnd()
{
  // Record completed message.
  this->Internal->MessageIndexes.push_back(this->Internal->StartIndex);
  this->Internal->StartIndex = vtkClientServerStreamInternals::InvalidStartIndex;
}

//----------------------------------------------------------------------------
unsigned char*
vtkClientServerStream::ParseType(int order, unsigned char* data,
                                 unsigned char* begin, unsigned char* end,
                                 vtkClientServerStream::Types* type)
{
  // Read this type identifier.
  vtkTypeUInt32 tp;
  if(data > end-sizeof(tp))
    {
    /* ERROR */
    return 0;
    }
  this->PerformByteSwap(order, data, 1, sizeof(tp));
  memcpy(&tp, data, sizeof(tp));
  *type = static_cast<vtkClientServerStream::Types>(tp);

  // Record the start of this type and optional value.
  this->Internal->ValueOffsets.push_back(data - begin);

  // Return the position after the type identifier.
  return data + sizeof(vtkTypeUInt32);
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseValue(int order,
                                                 unsigned char* data,
                                                 unsigned char* end,
                                                 unsigned int wordSize)
{
  // Byte-swap the value.
  if(data > end-wordSize)
    {
    /* ERROR */
    return 0;
    }
  this->PerformByteSwap(order, data, 1, wordSize);

  // Return the position after the value.
  return data+wordSize;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseArray(int order,
                                                 unsigned char* data,
                                                 unsigned char* end,
                                                 unsigned int wordSize)
{
  // Read the array length.
  vtkTypeUInt32 length;
  if(data > end-sizeof(length))
    {
    /* ERROR */
    return 0;
    }
  this->PerformByteSwap(order, data, 1, sizeof(length));
  memcpy(&length, data, sizeof(length));
  data += sizeof(length);

  // Calculate the size of the array data.
  vtkTypeUInt32 size = length*wordSize;

  // Byte-swap the array data.
  if(data > end-size)
    {
    /* ERROR */
    return 0;
    }
  this->PerformByteSwap(order, data, length, wordSize);

  // Return the position after the array.
  return data+size;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseString(int order,
                                                  unsigned char* data,
                                                  unsigned char* end)
{
  // Read the string length.
  vtkTypeUInt32 length;
  if(data > end-sizeof(length))
    {
    /* ERROR */
    return 0;
    }
  this->PerformByteSwap(order, data, 1, sizeof(length));
  memcpy(&length, data, sizeof(length));
  data += sizeof(length);

  // Skip the string data.  It does not need swapping.
  if(data > end-length)
    {
    /* ERROR */
    return 0;
    }

  // Return the position after the string.
  return data+length;
}

//----------------------------------------------------------------------------
unsigned char* vtkClientServerStream::ParseStream(int order,
                                                  unsigned char* data,
                                                  unsigned char* end)
{
  // Stream data are represented as an array of bytes.
  return this->ParseArray(order, data, end, 1);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PerformByteSwap(int dataByteOrder,
                                            unsigned char* data,
                                            unsigned int numWords,
                                            unsigned int wordSize)
{
  char* ptr = reinterpret_cast<char*>(data);
  if(dataByteOrder == vtkClientServerStream::BigEndian)
    {
    switch (wordSize)
      {
      case 1: break;
      case 2: vtkByteSwap::Swap2BERange(ptr, numWords); break;
      case 4: vtkByteSwap::Swap4BERange(ptr, numWords); break;
      case 8: vtkByteSwap::Swap8BERange(ptr, numWords); break;
      default: break;
      }
    }
  else
    {
    switch (wordSize)
      {
      case 1: break;
      case 2: vtkByteSwap::Swap2LERange(ptr, numWords); break;
      case 4: vtkByteSwap::Swap4LERange(ptr, numWords); break;
      case 8: vtkByteSwap::Swap8LERange(ptr, numWords); break;
      default: break;
      }
    }
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetNumberOfMessages() const
{
  return static_cast<int>(this->Internal->MessageIndexes.size());
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetNumberOfValues(int message) const
{
  if(!this->Internal->Invalid &&
     message >= 0 && message < this->GetNumberOfMessages())
    {
    if(message+1 < this->GetNumberOfMessages())
      {
      // Requested message is not the last message.  Use the beginning
      // of the next message to find its length.
      return (this->Internal->MessageIndexes[message+1] -
              this->Internal->MessageIndexes[message]);
      }
    else if(this->Internal->StartIndex !=
            vtkClientServerStreamInternals::InvalidStartIndex)
      {
      // Requested message is the last completed message, but there is
      // a partial message in progress.  Use the beginning of the next
      // partial message to find this message's length.
      return (this->Internal->StartIndex -
              this->Internal->MessageIndexes[message]);
      }
    else
      {
      // Requested message is the last message.  Use the length of the
      // value indices array to find the message's length.
      return (this->Internal->ValueOffsets.size() -
              this->Internal->MessageIndexes[message]);
      }
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
const unsigned char*
vtkClientServerStream::GetValue(int message, int value) const
{
  if(value >= 0 && value < this->GetNumberOfValues(message))
    {
    // Get the index to the beginning of the requested message.
    vtkClientServerStreamInternals::ValueOffsetsType::size_type index =
      this->Internal->MessageIndexes[message];

    // Return a pointer to the value-th value in the message.
    const unsigned char* data = &*this->Internal->Data.begin();
    return data + this->Internal->ValueOffsets[index + value];
    }
  else
    {
    return 0;
    }
}

//----------------------------------------------------------------------------
// Templates to find argument size within a stream.
template <class T> size_t vtkClientServerStreamValueSize(T*){return sizeof(T);}
template <class T>
size_t vtkClientServerStreamArraySize(const unsigned char* data, T*)
{
  // Get the length of the value in the stream.
  vtkTypeUInt32 len;
  memcpy(&len, data, sizeof(len));
  return sizeof(len) + len*sizeof(T);
}

vtkClientServerStream::Argument
vtkClientServerStream::GetArgument(int message, int argument) const
{
  // Prepare a return value.
  vtkClientServerStream::Argument result = {0, 0};

  // Get a pointer to the type/value pair in the stream.
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Store the starting location of the value.
    result.Data = data;

    // Get the type of the value in the stream.
    vtkTypeUInt32 tp;
    memcpy(&tp, data, sizeof(tp));
    data += sizeof(tp);

    // Find the length of the argument's data based on its type.
    switch(tp)
      {
      VTK_CSS_TEMPLATE_MACRO(value, result.Size = sizeof(tp) +
                             vtkClientServerStreamValueSize(T));
      VTK_CSS_TEMPLATE_MACRO(array, result.Size = sizeof(tp) +
                             vtkClientServerStreamArraySize(data, T));
      case vtkClientServerStream::id_value:
        {
        result.Size = sizeof(tp) + sizeof(vtkClientServerID().ID);
        } break;
      case vtkClientServerStream::string_value:
        {
        // A string is represented as an array of 1 byte values.
        vtkTypeUInt8* T = 0;
        result.Size = sizeof(tp) + vtkClientServerStreamArraySize(data, T);
        } break;
      case vtkClientServerStream::vtk_object_pointer:
        {
        result.Size = sizeof(tp) + sizeof(vtkObjectBase*);
        } break;
      case vtkClientServerStream::stream_value:
        {
        // A stream is represented as an array of 1 byte values.
        vtkTypeUInt8* T = 0;
        result.Size = sizeof(tp) + vtkClientServerStreamArraySize(data, T);
        } break;
      case vtkClientServerStream::LastResult:
        {
        result.Size = sizeof(tp);
        } break;
      case vtkClientServerStream::End:
      default:
        result.Data = 0;
        break;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
vtkClientServerStream::Commands
vtkClientServerStream::GetCommand(int message) const
{
  // The first value in a message is always the command.
  if(const unsigned char* data = this->GetValue(message, 0))
    {
    // Retrieve the command value and convert it to the proper type.
    vtkTypeUInt32 cmd;
    memcpy(&cmd, data, sizeof(cmd));
    if(cmd < vtkClientServerStream::EndOfCommands)
      {
      return static_cast<vtkClientServerStream::Commands>(cmd);
      }
    }
  return vtkClientServerStream::EndOfCommands;
}

//----------------------------------------------------------------------------
int vtkClientServerStream::GetNumberOfArguments(int message) const
{
  // Arguments do not include the Command or End markers.
  if(int numValues = this->GetNumberOfValues(message))
    {
    return numValues - 2;
    }
  else
    {
    return -1;
    }
}

//----------------------------------------------------------------------------
vtkClientServerStream::Types
vtkClientServerStream::GetArgumentType(int message, int argument) const
{
  // Get a pointer to the type/value pair in the stream.
  if(const unsigned char* data = this->GetValue(message, 1+argument))
    {
    // Get the type of the value in the stream and convert it.
    vtkTypeUInt32 type;
    memcpy(&type, data, sizeof(type));
    if(type < vtkClientServerStream::End)
      {
      return static_cast<vtkClientServerStream::Types>(type);
      }
    }
  return vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
// Map from the vtkClientServerStream::Types enumeration to strings.
// This must be kept in-sync with the enumeration.
static const char* const vtkClientServerStreamTypeNames[] =
{
  "int8_value", "int8_array",
  "int16_value", "int16_array",
  "int32_value", "int32_array",
  "int64_value", "int64_array",
  "uint8_value", "uint8_array",
  "uint16_value", "uint16_array",
  "uint32_value", "uint32_array",
  "uint64_value", "uint64_array",
  "float32_value", "float32_array",
  "float64_value", "float64_array",
  "string_value",
  "id_value",
  "vtk_object_pointer",
  "LastResult",
  "End",
  0
};

//----------------------------------------------------------------------------
const char*
vtkClientServerStream
::GetStringFromType(vtkClientServerStream::Types type)
{
  // Lookup the type if it is in range.
  if(type >= vtkClientServerStream::int8_value &&
     type <= vtkClientServerStream::End)
    {
    return vtkClientServerStreamTypeNames[type];
    }
  else
    {
    return "unknown";
    }
}

//----------------------------------------------------------------------------
vtkClientServerStream::Types
vtkClientServerStream::GetTypeFromString(const char* name)
{
  // Find a string matching the given name.
  for(int t = vtkClientServerStream::int8_value;
      name && t < vtkClientServerStream::End; ++t)
    {
    if(strcmp(vtkClientServerStreamTypeNames[t], name) == 0)
      {
      return static_cast<vtkClientServerStream::Types>(t);
      }
    }
  return vtkClientServerStream::End;
}

//----------------------------------------------------------------------------
// Map from the vtkClientServerStream::Commands enumeration to strings.
// This must be kept in-sync with the enumeration.
static const char* const vtkClientServerStreamCommandNames[] =
{
  "New", "Invoke", "Delete", "Assign",
  "Reply", "Error", "EndOfCommands",
  0
};

//----------------------------------------------------------------------------
const char*
vtkClientServerStream
::GetStringFromCommand(vtkClientServerStream::Commands cmd)
{
  // Lookup the command if it is in range.
  if(cmd >= vtkClientServerStream::New &&
     cmd <= vtkClientServerStream::EndOfCommands)
    {
    return vtkClientServerStreamCommandNames[cmd];
    }
  else
    {
    return "unknown";
    }
}

//----------------------------------------------------------------------------
vtkClientServerStream::Commands
vtkClientServerStream::GetCommandFromString(const char* name)
{
  // Find a string matching the given name.
  for(int c = vtkClientServerStream::New;
      name && c < vtkClientServerStream::EndOfCommands; ++c)
    {
    if(strcmp(vtkClientServerStreamCommandNames[c], name) == 0)
      {
      return static_cast<vtkClientServerStream::Commands>(c);
      }
    }
  return vtkClientServerStream::EndOfCommands;
}

//----------------------------------------------------------------------------
// Function to print a value argument.
template <class T>
void vtkClientServerStreamPrintValue(const vtkClientServerStream* self,
                                     ostream& os, int m, int a, T*)
{
  typedef VTK_CSS_TYPENAME vtkClientServerTypeTraits<T>::PrintType PrintType;
  T arg;
  self->GetArgument(m, a, &arg);
  vtkClientServerStream::Types type = self->GetArgumentType(m, a);
  os << "Argument " << a << " = " << self->GetStringFromType(type)
     << " {" << static_cast<PrintType>(arg) << "}\n";
}

//----------------------------------------------------------------------------
// Function to print an array argument.
template <class T>
void vtkClientServerStreamPrintArray(const vtkClientServerStream* self,
                                     ostream& os, int m, int a, T*)
{
  typedef VTK_CSS_TYPENAME vtkClientServerTypeTraits<T>::PrintType PrintType;
  vtkTypeUInt32 length;
  self->GetArgumentLength(m, a, &length);
  T* arg = new T[length];
  self->GetArgument(m, a, arg, length);
  vtkClientServerStream::Types type = self->GetArgumentType(m, a);
  os << "Argument " << a << " = " << self->GetStringFromType(type) << " {";
  const char* space = "";
  for(vtkTypeUInt32 i=0; i < length; ++i)
    {
    os << space << static_cast<PrintType>(arg[i]);
    space = " ";
    }
  os << "}\n";
  delete [] arg;
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Print(ostream& os) const
{
  vtkIndent indent;
  this->Print(os, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::Print(ostream& os, vtkIndent indent) const
{
  for(int m=0; m < this->GetNumberOfMessages(); ++m)
    {
    this->PrintMessage(os, m, indent);
    }
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintMessage(ostream& os, int message) const
{
  vtkIndent indent;
  this->PrintMessage(os, message, indent);
}

//----------------------------------------------------------------------------
void vtkClientServerStream::PrintMessage(ostream& os, int message,
                                         vtkIndent indent) const
{
  os << indent << "Message " << message << " = ";
  os << this->GetStringFromCommand(this->GetCommand(message)) << "\n";
  for(int a=0; a < this->GetNumberOfArguments(message); ++a)
    {
    vtkIndent indent2 = indent.GetNextIndent();
    os << indent2;
    switch(this->GetArgumentType(message, a))
      {
      VTK_CSS_TEMPLATE_MACRO(value, vtkClientServerStreamPrintValue
                             (this, os, message, a, T));
      VTK_CSS_TEMPLATE_MACRO(array, vtkClientServerStreamPrintArray
                             (this, os, message, a, T));
      case vtkClientServerStream::string_value:
        {
        const char* arg;
        this->GetArgument(message, a, &arg);
        os << "Argument " << a << " = string_value ";
        if(arg)
          {
          os << "{" << arg << "}\n";
          }
        else
          {
          os << "(null)\n";
          }
        } break;
      case vtkClientServerStream::stream_value:
        {
        vtkClientServerStream arg;
        int result = this->GetArgument(message, a, &arg);
        os << "Argument " << a << " = stream_value ";
        if(result)
          {
          vtkIndent indent3 = indent2.GetNextIndent();
          os << "{\n";
          arg.Print(os, indent3);
          os << indent3 << "}\n";
          }
        else
          {
          os << "invalid\n";
          }
        } break;
      case vtkClientServerStream::id_value:
        {
        vtkClientServerID arg;
        this->GetArgument(message, a, &arg);
        os << "Argument " << a << " = id_value {" << arg.ID << "}\n";
        } break;
      case vtkClientServerStream::vtk_object_pointer:
        {
        vtkObjectBase* arg;
        this->GetArgument(message, a, &arg);
        os << "Argument " << a << " = vtk_object_pointer ";
        if(arg)
          {
          os << "{" << arg->GetClassName() << " (" << arg << ")}\n";
          }
        else
          {
          os << "(null)\n";
          }
        } break;
      case vtkClientServerStream::LastResult:
        {
        os << "Argument " << a << " = LastResult\n";
        } break;
      default:
        {
        os << "Argument " << a << " = invalid\n";
        } break;
      }
    }
}
